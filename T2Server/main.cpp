#include <SFML\Graphics.hpp>
#include <SFML\Network.hpp>
#include <string>
#include <thread>
#include <mutex>
#include <iostream>
#include <vector>
#include <list>

#define MAX_MENSAJES 30
#define MAX_MSJ_SIZE 128

std::mutex mu;
enum conexionType { blockThread, nonBlock, SockSelector };

void disconnectSock(sf::TcpSocket* socket, sf::SocketSelector* selector, std::list<sf::TcpSocket*>* clients);

void sendAll(std::list<sf::TcpSocket*>* clients, sf::SocketSelector* selector, std::string msj) {
	for (std::list<sf::TcpSocket*>::iterator it = clients->begin(); it != clients->end(); it++) {
		sf::TcpSocket* client = *it;
		sf::Socket::Status status = client->send(msj.c_str(), msj.length());
		if (status == sf::Socket::Disconnected) {
			disconnectSock(client, selector, clients);
		}
	}
}

void disconnectSock(sf::TcpSocket* socket, sf::SocketSelector* selector, std::list<sf::TcpSocket*>* clients) {
	selector->remove(*socket);
	socket->disconnect();
	sendAll(clients, selector, "Un cliente se ha desconectado\n");
	std::cout << "Cliente desconectado\n";
}


void receive(std::string msj,sf::TcpSocket* client ,std::list<sf::TcpSocket*>* clients, sf::SocketSelector* selector) {
	//comprovamos que no sea un mensaje especial
	if (msj[1] == '/') {
		//std::cout << "Comando especial\n";
		if (msj == ">/exit") {
			disconnectSock(client, selector, clients);
		}
	}
	else {
		sendAll(clients, selector, msj);
	}
}

int main()
{
	//inicializamos server
	std::thread t1;
	sf::TcpListener listener;
	sf::SocketSelector selector;
	char buffer[MAX_MSJ_SIZE];
	std::size_t bytesReceived;
	//creamos una lista para guardar los sockets
	std::list<sf::TcpSocket*> clients;

	//escuchamos si el cliente se quiere conectar
	sf::Socket::Status status = listener.listen(5000);

	selector.add(listener);

/*
	sf::Vector2i screenDimensions(800, 600);

	sf::RenderWindow window;
	window.create(sf::VideoMode(screenDimensions.x, screenDimensions.y), "Chat");

	sf::Font font;
	if (!font.loadFromFile("calibri.ttf"))
	{
		std::cout << "Can't load the font file" << std::endl;
	}

	sf::String mensaje = " >";

	sf::Text chattingText(mensaje, font, 14);
	chattingText.setFillColor(sf::Color(0, 160, 0));
	chattingText.setStyle(sf::Text::Bold);


	sf::Text text(mensaje, font, 14);
	text.setFillColor(sf::Color(0, 160, 0));
	text.setStyle(sf::Text::Bold);
	text.setPosition(0, 560);

	sf::RectangleShape separator(sf::Vector2f(800, 5));
	separator.setFillColor(sf::Color(200, 200, 200, 255));
	separator.setPosition(0, 550);
*/
	sf::String mensaje = " >";
	bool isActive = true;
	while (isActive)
	{
		/*sf::Event evento;
		while (window.pollEvent(evento))
		{
			switch (evento.type)
			{
			case sf::Event::Closed:
				window.close();
				break;
			case sf::Event::KeyPressed:
				if (evento.key.code == sf::Keyboard::Escape)
					window.close();
				else if (evento.key.code == sf::Keyboard::Return)
				{
				}
				break;
			case sf::Event::TextEntered:
				if (evento.text.unicode >= 32 && evento.text.unicode <= 126)
					mensaje += (char)evento.text.unicode;
				else if (evento.text.unicode == 8 && mensaje.getSize() > 0)
					mensaje.erase(mensaje.getSize() - 1, mensaje.getSize());
				break;
			}
		}*/
		//recibir datos
		if (selector.wait()) {
			//miramos el listener
			if (selector.isReady(listener)) {
				//creamos un socket para el nuevo cliente
				sf::TcpSocket* client = new sf::TcpSocket;
				//si el accept se hace bien
				sf::Socket::Status status = listener.accept(*client);
				if (status == sf::Socket::Done) {
					//añadimos al nuevo cliente a la lista
					clients.push_back(client);
					//añadimos el cliente al selector
					selector.add(*client);
					//avisamos de que se ha conectado
					sendAll(&clients, &selector,std::string("Nuevo cliente en el chat"));
					std::cout << "Cliente aceptado\n";
				}
				else if(status== sf::Socket::Disconnected){
					disconnectSock(client, &selector, &clients);
				}
			}
			else {
				//comprovamos los sockets de los clientes
				for (std::list<sf::TcpSocket*>::iterator it = clients.begin(); it != clients.end(); it++) {
					sf::TcpSocket* client = *it;
					if (selector.isReady(*client)) {
						//recivimos los datos del cliente
						if (client->receive(buffer, MAX_MSJ_SIZE, bytesReceived) == sf::Socket::Done) {
							buffer[bytesReceived] = '\0';
							receive(buffer,client,&clients,&selector);
						}
					}
				}
			}
		}
	}
	//socket.disconnect();
	for (std::list<sf::TcpSocket*>::iterator it = clients.begin(); it != clients.end(); it++) {
		sf::TcpSocket* client = *it;
		client->disconnect();
		selector.remove(*client);
	}
	listener.close();
	t1.join();
}
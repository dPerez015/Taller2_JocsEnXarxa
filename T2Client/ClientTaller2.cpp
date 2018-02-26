#include <SFML\Graphics.hpp>
#include <SFML\Network.hpp>
#include <string>
#include <thread>
#include <mutex>
#include <iostream>
#include <vector>

//recibir avisos de clientes que se conectan y desconectan
//enviar y recibir mensajes

//#define MAX_MENSAJES 30
std::mutex mu;
bool connected = false;

void sendString(sf::TcpSocket* socket, std::string msj) {
	sf::Socket::Status status = socket->send(msj.c_str(), msj.length());
	if (status != sf::Socket::Done) {
		std::cout << "Error al enviar\n";
	}
}

void receive(sf::TcpSocket* socket, std::vector<std::string>* aMensajes) {
	
	char buffer[100];
	std::size_t bytesReceived;
	while (connected) {
		sf::Socket::Status status = socket->receive(&buffer, 100, bytesReceived);
		if (status == sf::Socket::Status::Disconnected) {
			connected = false;
			mu.lock();
			aMensajes->push_back("Desconectado del servidor");
			mu.unlock();
		}
		else if (status == sf::Socket::Status::Done) {
			buffer[bytesReceived] = '\0';
			mu.lock();
			aMensajes->push_back(std::string(buffer));
			mu.unlock();
		}
	}
}

int main()
{
	
	std::thread t1;
	std::vector<std::string> aMensajes;

	//establecimiento de conexion
	sf::TcpSocket socket;
	sf::Socket::Status status = socket.connect("127.0.0.1", 5000, sf::seconds(5.f));

	if (status != sf::Socket::Status::Done) {
		std::cout << "Problema al establecer conexión.\n";
	}
	else {
		std::cout << "Conectado\n";
		connected = true;
		//inicializamos el thread
		t1 = std::thread(&receive, &socket, &aMensajes);
	}

	//Codigo de display y enviar/recivir datos

	sf::Vector2i screenDimensions(800, 600);

	sf::RenderWindow window;
	window.create(sf::VideoMode(screenDimensions.x, screenDimensions.y), "Chat");

	sf::Font font;
	if (!font.loadFromFile("calibril.ttf"))
	{
		std::cout << "Can't load the font file" << std::endl;
	}

	sf::String mensaje = ">";

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

	while (window.isOpen())
	{
		sf::Event evento;
		while (window.pollEvent(evento))
		{
			switch (evento.type)
			{
			case sf::Event::Closed:
				sendString(&socket, ">/exit");
				window.close();
				break;
			case sf::Event::KeyPressed:
				if (evento.key.code == sf::Keyboard::Escape)
					window.close();
				else if (evento.key.code == sf::Keyboard::Return)
				{
					if (mensaje == ">/exit") {
						sendString(&socket, mensaje);
						//window.close();
						std::cout << "sale\n";
						/*std::string msj = "Cliente desconectado\n";
						sendString(&socket, msj);*/
					}
					else {
						//SEND
						sendString(&socket, mensaje);

						if (aMensajes.size() > 25)
						{
							aMensajes.erase(aMensajes.begin(), aMensajes.begin() + 1);
						}

						mensaje = ">";
					}
				}
				break;
			case sf::Event::TextEntered:
				if (evento.text.unicode >= 32 && evento.text.unicode <= 126)
					mensaje += (char)evento.text.unicode;
				else if (evento.text.unicode == 8 && mensaje.getSize() > 0)
					mensaje.erase(mensaje.getSize() - 1, mensaje.getSize());
				break;
			}
			if (!connected) {
				window.close();
			}
		}
		
		window.draw(separator);
		for (size_t i = 0; i < aMensajes.size(); i++)
		{
			std::string chatting = aMensajes[i];
			chattingText.setPosition(sf::Vector2f(0, 20 * i));
			chattingText.setString(chatting);
			window.draw(chattingText);
		}
		std::string mensaje_ = mensaje + "_";
		text.setString(mensaje_);
		window.draw(text);

		window.display();
		window.clear();
	}
	socket.disconnect();
	t1.join();
}
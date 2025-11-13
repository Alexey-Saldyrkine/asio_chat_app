# asio_chat_app
This a simple terminal based chat app. It works on a client-server model, using ncurses for the terminal gui.

For the networking boost::Asio tcp sockets are used. This allows the server and clients to work asynchronously, giving a smooth experience to the user.

The serializer is build on top of [cereal](https://github.com/USCiLab/cereal). The serializer can be easily extended to support any data structure that has a cereal serialization function and is registered to the serializer as a template parameter. Type ids are automatically assigned during compilation, they are used to tell the decoder the type the received bytes are.

When a user firsts connects to the server it is sends its name to the server, so that the server will be able to spread it to the other users. The server send the users client its unique id on the network, as well as a list of other users names and their ids. When a new user joins its name and id is sent to the rest of the clients. 

When a user wants to send a message to another user they select their name from the list of other users and type out their message. The client serializes the message and packages it with meta data about the message, its size, sender and receiver ids, and type id. The server receives this and passes it to the receiver, at which the receiving client decode and displays it to the end user.

When the app is launched with no arguments, then it runs as the server, when provided one argument is is launched as a client with the argument being the clients name.

Example:

1. Top right is the server, the rest are freshly created clients.

![img1](/imgs/img1.png)

As you can see the server shows the log of all actions. Its shows that the three users connected and sent their names to the server. 

The clients each have three windows. The biggest is the output window which shows all the received messages. It shows that the client has received its network id from the server (id 0). The top right window is the text input window, where the user types out their message. The bottom right box is the network users window, which shows all the other users on the network. It shows the three connected users names and their ids.

2. The user Harper (top right) sends a message to the Skyler user (bottom left). You can see which user they are sending to using the highlighted name in the user box. Skyler also sent a message to Harper, while Dakota (bottom right) is preparing to send a message to Harper. Dakota did not receive the other messages as he was not the recipient.

![img2](/imgs/img2.png)

3. Dakota sends a message to Harper, and Skyler sends a message to Dakota.

![img3](/imgs/img3.png)

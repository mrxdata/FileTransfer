# File Transfer Console Application

This is a C++ console application for secure file transfer between a client and server, using OpenSSL for encryption. Designed for Windows OS, it supports essential file transfer commands with a simple, Linux-like command-line interface.

## Requirements
- **Operating System**: Windows
- **C++ Version**: 20
- **OpenSSL Version**: 3.3.2

## Features
- **Client/Server Architecture**: Allows file transfers between two endpoints.
- **Encryption**: Utilizes OpenSSL for secure data transfer.

## Usage
Launch the application in either server or client mode. Use the following commands to interact with the application.

### Commands

- **server**  
  Runs the server side of the application.  
  If no port is specified, the application defaults to port 22.  
  Example:  
  - Run server on the default port: `server`
  - Run server on a specified port: `server -port 12345`

- **connect**  
  Connects the client to a specified server address and port.  
  If no port is specified, the application defaults to port 22.  
  Example:  
  - Connect to a server on the default port: `connect 127.0.0.1`
  - Connect to a server on a specified port: `connect 127.0.0.1:12345`

- **send**  
  Sends a file from the client’s current directory to the server.  
  Usage: `send <filename>`  
  Example: `send data.txt`

- **cd**  
  Changes the current directory.  
  Usage: `cd <path>`  
  Example: `cd path/to/directory`

- **ls**  
  Lists all files in the current directory.  
  Usage: `ls`



# File Transfer Console Application

This is a C++ console application for secure file transfer between a client and server, using OpenSSL for encryption. 
Designed for Windows OS, it supports essential file transfer commands with a simple, Linux-like command-line interface.

## Requirements

- **Operating System**: Windows
- **C++ Version**: 20
- **OpenSSL Version**: 3.3.2

## Features

- **Client/Server Architecture**: Facilitates file transfers between two endpoints.
- **Encryption**: Utilizes OpenSSL for secure data transfer.
- **Certificate Management**: Includes tools for certificate generation, saving, and validation using OpenSSL.
- **Command Line Interface**: Supports Linux-like commands for ease of use.

## Usage

Launch the application in either server or client mode. Use the following commands to interact with the application. 
To create a server or connect to a server, you first need to generate ("crtkey gen") or use a PEM file ("crtkey use") that must contain a private key and a certificate.

### Commands

- **server**  
Runs the server side of the application.  
**Usage**: `server <port>`  
**Example**: `server 12345`

- **connect**  
Connects the client to a specified server address and port.  
If no port is specified, the application defaults to port 22.  
**Example**:  
  - Connect to a server on the default port: `connect 127.0.0.1`  
  - Connect to a server on a specified port: `connect 127.0.0.1:12345`

- **send**  
Sends a file from the client’s current directory to the server.  
**Usage**: `send <filename>`  
**Example**: `send data.txt`

- **cd**  
Changes the current directory on the client side.  
**Usage**: `cd <path>`  
**Example**: `cd path/to/directory`

- **ls**  
Lists all files in the client’s current directory.  
**Usage**: `ls`

- **crtkey**  
Manages client certificates.  
**Options**:  
  - `gen`: Generates new certificates.  
  - `save`: Saves current certificates to a PEM file.      
**Example**:
  - Generate certificates: `crtkey gen`  
  - Save certificates: `crtkey save`  
  - Generate and save certificates: `crtkey gen save`

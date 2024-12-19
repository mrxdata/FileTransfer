#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

class CertManager {
public:
	static SSL_CTX* setupSSLContext(bool isServer);
	static inline std::string certificate = "";
	static inline std::string privateKey = "";
	static bool usePEM(const std::string& filepath);
	static bool savePEM(const std::string& filename);
	static void generatePEM();
};

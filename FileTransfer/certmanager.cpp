#include "certmanager.h"

void CertManager::generatePEM() {
	EVP_PKEY* pkey = EVP_PKEY_new();
	if (!pkey) {
		std::cerr << "Error: Failed to create EVP_PKEY.\n";
		return;
	}

	EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
	if (!ctx) {
		std::cerr << "Error: Failed to create EVP_PKEY_CTX.\n";
		EVP_PKEY_free(pkey);
		return;
	}

	if (EVP_PKEY_keygen_init(ctx) <= 0 || EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0 || EVP_PKEY_keygen(ctx, &pkey) <= 0) {
		std::cerr << "Error: Failed to generate RSA key.\n";
		EVP_PKEY_CTX_free(ctx);
		EVP_PKEY_free(pkey);
		return;
	}

	EVP_PKEY_CTX_free(ctx);

	BIO* keyBio = BIO_new(BIO_s_mem());
	if (!PEM_write_bio_PrivateKey(keyBio, pkey, nullptr, nullptr, 0, nullptr, nullptr)) {
		std::cerr << "Error: Failed to write private key to BIO.\n";
		EVP_PKEY_free(pkey);
		BIO_free(keyBio);
		return;
	}

	BUF_MEM* keyBuffer;
	BIO_get_mem_ptr(keyBio, &keyBuffer);
	CertManager::privateKey = std::string(keyBuffer->data, keyBuffer->length);

	BIO_free(keyBio);

	X509* cert = X509_new();
	if (!cert) {
		std::cerr << "Error: Failed to create X509 certificate.\n";
		EVP_PKEY_free(pkey);
		return;
	}

	X509_set_version(cert, 2);
	ASN1_INTEGER_set(X509_get_serialNumber(cert), 1);
	X509_gmtime_adj(X509_get_notBefore(cert), 0);
	X509_gmtime_adj(X509_get_notAfter(cert), 31536000L);
	X509_set_pubkey(cert, pkey);

	X509_NAME* name = X509_get_subject_name(cert);
	X509_NAME_add_entry_by_txt(name, "C", MBSTRING_ASC, (unsigned char*)"RU", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "O", MBSTRING_ASC, (unsigned char*)"dv3", -1, -1, 0);
	X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char*)"github.com/mrxdata", -1, -1, 0);

	X509_set_issuer_name(cert, name);

	if (!X509_sign(cert, pkey, EVP_sha256())) {
		std::cerr << "Error: Failed to sign certificate.\n";
		X509_free(cert);
		EVP_PKEY_free(pkey);
		return;
	}

	BIO* certBio = BIO_new(BIO_s_mem());
	if (!PEM_write_bio_X509(certBio, cert)) {
		std::cerr << "Error: Failed to write certificate to BIO.\n";
		X509_free(cert);
		EVP_PKEY_free(pkey);
		BIO_free(certBio);
		return;
	}

	BUF_MEM* certBuffer;
	BIO_get_mem_ptr(certBio, &certBuffer);
	CertManager::certificate = std::string(certBuffer->data, certBuffer->length);

	BIO_free(certBio);
	X509_free(cert);
	EVP_PKEY_free(pkey);

	std::cout << "Key and certificate generated and stored in memory.\n";
}

bool CertManager::savePEM(const std::string& filename) {
	if (privateKey.empty() || certificate.empty()) {
		std::cerr << "Error: No key or certificate in memory. Use 'crtkey gen' first.\n";
		return false;
	}

	std::string fullFilename = filename;
	if (fullFilename.find(".pem") == std::string::npos) {
		fullFilename += ".pem";
	}

	if (std::filesystem::exists(fullFilename)) {
		char choice;
		std::cout << "File already exists. Do you want to overwrite it? (y/n): ";
		std::cin >> choice;
		if (choice != 'y' && choice != 'Y') {
			std::cout << "Operation cancelled.\n";
			return false;
		}
	}

	std::ofstream file(fullFilename);
	if (!file.is_open()) {
		std::cerr << "Error: Unable to open file for writing: " << fullFilename << "\n";
		return false;
	}

	file << privateKey << "\n" << certificate;
	file.close();

	std::cout << "PEM file saved as: " << fullFilename << "\n";
	return true;
}

bool CertManager::usePEM(const std::string& filepath) {
	std::filesystem::path path(filepath);
	if (path.is_relative()) {
		path = std::filesystem::current_path() / path;
	}

	if (!std::filesystem::exists(path)) {
		std::cerr << "Error: File does not exist: " << path << "\n";
		return false;
	}

	std::ifstream file(path, std::ios::binary);
	if (!file.is_open()) {
		std::cerr << "Error: Unable to open file: " << path << "\n";
		return false;
	}

	std::ostringstream buffer;
	buffer << file.rdbuf();
	const std::string content = buffer.str();

	auto pos = content.find("-----BEGIN CERTIFICATE-----");
	if (pos == std::string::npos) {
		std::cerr << "Error: Invalid PEM file format.\n";
		return false;
	}

	privateKey = content.substr(0, pos);
	certificate = content.substr(pos);

	std::cout << "PEM file loaded into memory from: " << path.string() << "\n";
	return true;
}

SSL_CTX* CertManager::setupSSLContext(bool isServer) {
	SSL_CTX* ctx = nullptr;

	if (isServer) {
		ctx = SSL_CTX_new(TLS_server_method());
	}
	else {
		ctx = SSL_CTX_new(TLS_client_method());
	}

	if (!ctx) {
		std::cerr << "Failed to create SSL context" << std::endl;
		return nullptr;
	}

	BIO* certBio = BIO_new_mem_buf(CertManager::certificate.c_str(), -1);
	X509* cert = PEM_read_bio_X509(certBio, nullptr, nullptr, nullptr);
	if (!cert) {
		std::cerr << "Failed to load certificate" << std::endl;
		BIO_free(certBio);
		return nullptr;
	}

	if (SSL_CTX_use_certificate(ctx, cert) <= 0) {
		std::cerr << "Failed to set certificate" << std::endl;
		X509_free(cert);
		BIO_free(certBio);
		return nullptr;
	}

	X509_free(cert);
	BIO_free(certBio);

	BIO* keyBio = BIO_new_mem_buf(CertManager::privateKey.c_str(), -1);
	EVP_PKEY* privateKey = PEM_read_bio_PrivateKey(keyBio, nullptr, nullptr, nullptr);
	if (!privateKey) {
		std::cerr << "Failed to load private key" << std::endl;
		BIO_free(keyBio);
		return nullptr;
	}

	if (SSL_CTX_use_PrivateKey(ctx, privateKey) <= 0) {
		std::cerr << "Failed to set private key" << std::endl;
		EVP_PKEY_free(privateKey);
		BIO_free(keyBio);
		return nullptr;
	}

	EVP_PKEY_free(privateKey);
	BIO_free(keyBio);
	return ctx;
}


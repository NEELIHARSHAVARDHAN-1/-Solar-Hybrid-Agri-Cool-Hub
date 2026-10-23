/**
 * @file        certs.h
 * @brief       SSL/TLS Root CA Certificate storage interface.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Interface declaration exposing external linkages to public Root Certificate Authority 
 * (CA) validation keys. Preserves compilation decoupling by isolating implementation-specific 
 * raw cryptographic buffer arrays to the source compilation unit.
 * * @section Responsibilities
 * - Declares the global read-only character reference wrapper for secure sockets.
 * - Enforces zero-allocation symbol translation across peripheral networking modules.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

#ifndef CERTS_H
#define CERTS_H

// 1. Standard C/C++ Libraries
// (None required natively in global scope)

// 2. ESP-IDF / Core Arduino Libraries
// (None required natively in global scope)

// 3. Third-Party Libraries
// (Managed inside subsystem implementation layers)

// 4. Project Local Headers
// (None required natively in global scope)

// =====================================================
// Public External Cryptographic References
// =====================================================

/**
 * @brief  External reference to the PEM-encoded root CA certificate array.
 * * The actual certificate data is allocated in the corresponding certs.cpp 
 * file to maintain strict physical layer encapsulation and keep the header clean.
 */
extern const char root_ca[];

#endif // CERTS_H
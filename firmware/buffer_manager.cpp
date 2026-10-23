/**
 * @file        buffer_manager.cpp
 * @brief       Circular Telemetry Buffer for offline data storage.
 * @project     FactorySense Industrial IoT Firmware
 * @platform    ESP32-S3 (Arduino Framework)
 * * @section Description
 * Implementation of a non-volatile/RAM-bound First-In-First-Out (FIFO) circular ring buffer.
 * Caches outbound tracking payloads during transmission losses, handling continuous data production 
 * via automatic overwrites of historical entries upon hitting strict allocation limits.
 * * @section Responsibilities
 * - Manages queue tracking indices (`head`, `tail`) bound to an explicit boundary threshold (`BUFFER_SIZE`).
 * - Exposes structural checking properties to query cache densities.
 * - De-queues data streams sequentially when reliable infrastructure connections recover.
 * * @note        STRICT NON-FUNCTIONAL CLEANUP PASS. Do not alter execution flow,
 * timing sequences, or state variables.
 */

// 1. Standard C/C++ Libraries
// (None required natively in global scope)

// 2. ESP-IDF / Core Arduino Libraries
#include <Arduino.h>

// 3. Third-Party Libraries
// (Managed inside subsystem implementation layers)

// 4. Project Local Headers
#include "buffer_manager.h"

// =====================================================
// Internal Buffer State
// =====================================================

static String buffer[BUFFER_SIZE];

static int head = 0;   // Pointer to the oldest element
static int tail = 0;   // Pointer to the next insertion point
static int count = 0;  // Current number of stored elements

// =====================================================
// Initialization
// =====================================================

/**
 * @brief  Flushes local buffer memory blocks and standardizes runtime pointers.
 */
void initBuffer()
{
    head = 0;
    tail = 0;
    count = 0;

    Serial.println("[BUFFER] Initialized");
    Serial.print("[BUFFER] Capacity = ");
    Serial.println(BUFFER_SIZE);
}

// =====================================================
// Add Payload to Buffer
// =====================================================

/**
 * @brief  Pushes a fresh transactional tracking string directly into the ring structure array layout.
 * @param  payload Outbound telemetry string message data.
 * @note   If tracking density crosses allocation thresholds, the oldest record drops automatically.
 */
void addToBuffer(const String& payload)
{
    buffer[tail] = payload;

    // Move tail index circularly
    tail = (tail + 1) % BUFFER_SIZE;

    if (count < BUFFER_SIZE)
    {
        count++;
    }
    else
    {
        Serial.println("[BUFFER] Full - Oldest Dropped");
        
        // Move head to drop the oldest entry
        head = (head + 1) % BUFFER_SIZE;
    }
}

// =====================================================
// Check for Available Data
// =====================================================

/**
 * @brief  Verifies whether data tracking entries are cached in local structures.
 * @return true if elements reside in storage lines, false if the structure is empty.
 */
bool hasBufferedData()
{
    return (count > 0);
}

// =====================================================
// Retrieve Oldest Payload
// =====================================================

/**
 * @brief  Pulls the frontmost active text array block without clearing queue configurations.
 * @return String object carrying the earliest available raw transaction text block.
 */
String getBufferedData()
{
    if (count == 0)
    {
        return "";
    }

    return buffer[head];
}

// =====================================================
// Remove Oldest Payload
// =====================================================

/**
 * @brief  Pops the oldest entry out of active queues by stepping the index forwards circularly.
 */
void removeBufferedData()
{
    if (count == 0)
    {
        return;
    }

    // Move head index circularly to remove entry
    head = (head + 1) % BUFFER_SIZE;
    count--;
}

// =====================================================
// Get Current Buffer Occupancy
// =====================================================

/**
 * @brief  Queries the aggregate number of objects residing within the storage boundaries.
 * @return Integer tally representing the total loaded cache size.
 */
int getBufferedCount()
{
    return count;
}
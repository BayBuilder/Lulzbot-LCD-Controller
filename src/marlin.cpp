/*
 *
 * Marlin G-code sender
 *
 * Features:
 *   - Numbered G-code
 *   - Marlin XOR checksum
 *   - Sliding transmit window
 *   - "ok" acknowledgement handling
 *   - "Resend:N" recovery
 *   - G-code history buffer
 *   - Non-blocking serial receive
 *   - Response timeout
 *   - Marlin ADVANCED_OK parsing
 *   - Optional automatic window sizing from B/P
 *
 * Tested conceptually for Arduino-ESP32.
 *
 * IMPORTANT:
 *   This is a HOST. The ESP32 is connected to a Marlin printer.
 *
 * Set MARLIN_UART, MARLIN_BAUD_RATE, 
 * MARLIN_TX, MARLIN_RX, and MARLIN_DTR in config.h
 *
 */
#include <Arduino.h>
#include <HardwareSerial.h>

#include <cctype>
#include <config.h>
#include <cstdint>
#include <globals.h>
#include <marlin.h>


// MARLIN INCOMING SERIAL RESPONSE PROCESSING
// This is the main method that should be
// called repeatedly from main:loop()
void Marlin::process()
{
    static char response[MARLIN_RX_LENGTH];
    static size_t length = 0;
    static bool overflow = false;

    while (serial.available())
    {
        int c = serial.read();

        // Error or read non-ASCII character
        if (c < 0) {
            LOGN("Marlin: Received: ");
            LOG(c, HEX);
            return;
        }

        // Ignore CR
        if (c == '\r') continue;

        // End of line
        if (c == '\n')
        {
            response[length] = '\0';

            if (length > 0)
                processResponse(response);

            length = 0;
            overflow = false;
            continue;
        }

        // If there's room in the buffer for the incoming
        // character and a '\0' terminator, store it
        if (length < sizeof(response) - 1) {

          if (!overflow)
            response[length++] = (char)c;

        } else {

          // Response too long, ignore it and
          // additional incoming characters until the
          // next '\n' is received
          response[length] = '\0';
          LOG("Marlin: RX buffer full");
          LOGN("Buffer: ");
          LOG(response);
          overflow = true;
          length = 0;
        }
    } // while (serial.available()

    // Make sure we're still receiving responses
    checkTimeout();
}

// PROCESS ONE FULLY RECEIVED MARLIN RESPONSE
void Marlin::processResponse(char *line)
{
    LOGN("Marlin RX: ");
    LOG(line);

    // Marlin is still alive
    lastResponseTime = millis();

    // OK
    if (strncmp(line, "ok", 2) == 0)
    {
        parseAdvancedOK(line);

        if (outstanding > 0)
            outstanding--;

        busyState = false;
        return;
    }

    // RESEND
    // Examples:
    //   Resend:123
    //   Resend: 123
    if (strncmp(line, "Resend:", 7) == 0)
    {
        char *p = line + 7;

        // Skip leading whitespace
        while (*p == ' ' || *p == '\t')
            p++;

        uint32_t requestedLine = strtoul(p, nullptr, 10);
        lastResendLine = requestedLine;
        resendState = true;

        if (sender)
        {
            if (!sender->resend(requestedLine))
                errorState = true;
        }
        return;
    }

    // ERROR
    if (strncmp(line, "Error:", 6) == 0)
    {
        errorState = true;
        LOG("*** MARLIN ERROR ***");
        LOG(line);
        return;
    }

    // BUSY
    if (strncmp(line, "busy:", 5) == 0)
    {
        /* --- TO DO --- */
        // Reset timeout timer...
        busyState = true;
        return;
    }

    // ECHO
    // Ignore echo messages.
    if (strncmp(line, "echo:", 5) == 0)
    {
        return;
    }

    // T: (Temperature update)
    if (strncmp(line, " T:", 3) == 0) {
      //char *p = line + 3;

      readTemps(line);
      return;
    }
    #if 0

      readTemp(NozzTemp, sizeof(NozzTemp), p);

      p = strstr(line, "/");
      if (p != nullptr) {
        *p++ = ' ';
        readTemp(NozzTarget, sizeof(NozzTarget), p);
      }

      p = strstr(line, "B");
      if (p != nullptr) {
        p++;
        readTemp(BedTemp, sizeof(BedTemp), p);
      }

      p = strstr(line, "/");
      if (p != nullptr) {
        p++;
        readTemp(BedTarget, sizeof(BedTarget), p);
      }

      snprintf(line, MARLIN_RX_LENGTH, "Nozz: %s/%s Bed: %s/%s", NozzTemp,
               NozzTarget, BedTemp, BedTarget);
      lv_subject_set_string(temps, line);

      return;
    }
    #endif // 0
    // Other responses
    LOGN("Marlin: ");
    LOG(line);
}

void Marlin::readTemp(char *temp, size_t length, char *p) {

  // Skip leading whitespace
  while (*p == ' ' || *p == '\t') p++;

  // Copy up to length-1 chars from p into temp
  // Stop if p is a space
  memset(temp, '\0', length);
  while (*p != ' ' && length > 1) {

    *temp++ = *p++;
    length--;
  }
}

void Marlin::readTemps(char *line) {
  char *p;
  char *q;

  p = strstr(line, " T:");
  if (p != nullptr) {

    //Nozz temp
    p += 3;
    q = NozzTemp;
    while (isdigit(*p)) {
      *q++ = *p++;
    }
    *q = '\0';

    //Nozz target
    p = strstr(line, "/");
    if (p != nullptr) {
      *p++ = ' ';
      q = NozzTarget;
      while (isdigit(*p)) {
        *q++ = *p++;
      }
      *q = '\0';
    }

    //Bed temp
    p = strstr(line, " B:");
    if (p != nullptr) {
      p += 3;
      q = BedTemp;
      while (isdigit(*p)) {
        *q++ = *p++;
      }
      *q = '\0';
    }

    //Bed target
    p = strstr(line, "/");
    if (p != nullptr) {
      p++;;
      q = BedTarget;
      while (isdigit(*p)) {
        *q++ = *p++;
      }
      *q = '\0';
    }

    snprintf(line, MARLIN_RX_LENGTH, "Nozz: %s/%s Bed: %s/%s", NozzTemp,
             NozzTarget, BedTemp, BedTarget);
    lv_subject_set_string(temps, line);
  }
}

// Parse ADVANCED_OK
// Example:
//     ok N42 P15 B3
//
// P = free planner slots
// B = free command queue slots
void Marlin::parseAdvancedOK(char *response) {
  const char *p;

  // Read acknowledged line number
  p = strstr(response, " N");
  if (p != nullptr) {
    p++;
    uint32_t acknowledgedLine = strtoul(p + 1, nullptr, 10);
    (void)acknowledgedLine;
    advancedOK = true;
  }

  // Read free motion planner slots
  p = strstr(response, " P");
  if (p != nullptr) {
    p++;
    plannerFree = static_cast<uint16_t>(strtoul(p + 1, nullptr, 10));
    advancedOK = true;
  }

  // Read free command queue slots.
  p = strstr(response, " B");
  if (p != nullptr) {
    p++;
    bufferFree = static_cast<uint16_t>(strtoul(p + 1, nullptr, 10));
    advancedOK = true;
  }

  p = strstr(response, " T:");
  if (p != nullptr) {
    readTemps(response);
  }
}

// Calculate effective transmit window
uint16_t Marlin::effectiveWindow() const {

  // Without ADVANCED_OK, use predefined window size
  if (!MARLIN_ADVANCED_OK || !advancedOK)
    return MARLIN_WINDOW_SIZE;

  // If no buffers available, use smallest size
  if (bufferFree == 0)
    return 1;

  uint16_t window = bufferFree;

  // Limit window size to a reasonable value
  if (window > (MARLIN_HISTORY_SIZE - 2))
    window = MARLIN_HISTORY_SIZE - 2;

  return window;
}

// Timeout
void Marlin::checkTimeout(void) {

  // No commands are awaiting a response
  if (outstanding == 0)
    return;

  uint32_t now = millis();

  if (static_cast<uint32_t>(now - lastResponseTime) > MARLIN_TIMEOUT_MS) {
    if (!errorState) {
      LOG("Marlin: Response timeout");
    }
    errorState = true;
  }
}
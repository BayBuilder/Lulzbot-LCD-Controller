#ifndef MARLIN_H
#define MARLIN_H

#include <config.h>
#include <SdFat.h>

// FORWARD DECLARATIONS
class GcodeSender;

/* --- MARLIN COMMUNICATION CLASS --- */
//
// Handles the serial connection with the printer
//   * Sends commands received from the gcode sender
//   * Processes responses received from the printer
// Allows for command queuing, resending of failed
// commands

class Marlin
{
public:

    Marlin(HardwareSerial &serialPort)
        : serial(serialPort)
    {
    }

    // Initialize serial connection
    void begin(void) {
      // Initialize I/O pins & UART
      LOG("Initializing Marlin");

      pinMode(MARLIN_DTR_PIN, OUTPUT);
      serial.begin(MARLIN_BAUD_RATE, SERIAL_8N1, MARLIN_RX_PIN, MARLIN_TX_PIN);

      // Begin communication, reset state machine
      resetHardware();
      serial.print("\n");
      resetState();
    }

    // Electrically resets the printer MCU
    void resetHardware(void) {

      // DTR low ~1s to reset printer
      LOG("Marlin wakeup");
      digitalWrite(MARLIN_DTR_PIN, LOW);
      delay(1000);
      digitalWrite(MARLIN_DTR_PIN, HIGH);
    }

    // Attach G-code sender
    void attachSender(GcodeSender *senderPtr)
    {
        sender = senderPtr;
    }

    // Process incoming serial data
    // Call this continuously from loop().
    void process();

    // Can another command be transmitted?
    bool canSend() const
    {
        if (errorState) return false;
        if (resendState) return false;
        if (busyState) return false;

        return outstanding < effectiveWindow();
    }

    // Sends a completed command
    // N<line number><command>*<checksum>
    // N25 G28 X*34
    // Can be used to send non-numbered commands (rawSend)
    void send(char *buffer)
    {
        if (!canSend()) return;

        LOGN("Marlin_TX: ");
        LOG(buffer);
        serial.println(buffer);
        outstanding++;
        lastResponseTime = millis();
    }
    void sendRaw(char *buffer)
    {
        if (!canSend()) return;

        LOGN("Marlin_Raw: ");
        LOG(buffer);
        serial.println(buffer);
        lastResponseTime = millis();
    }

    // Number of commands waiting for "ok"
    uint16_t getOutstanding() const { return outstanding; }

    // Reset communication state
    void resetState()
    {
        outstanding = 0;
        resendState = false;
        busyState = false;
        errorState = false;
        lastResendLine = 0;
        lastResponseTime = millis();
    }

    // Called when sender seeks backward after Resend:N
    void resendStarted()
    {
        /*
         * Everything sent after the requested line is now
         * invalid and must be resent
         *
         * Marlin will restart its command stream
         * beginning with the requested line.
         */

        outstanding = 0;
        errorState = false;
        resendState = false;
    }

    // Error state
    bool hasError() const { return errorState; }
    void clearError() { errorState = false; }


private:

    HardwareSerial &serial;
    GcodeSender *sender = nullptr;
    uint16_t outstanding = 0;

    bool resendState = false;
    bool busyState = false;
    bool errorState = false;

    uint32_t lastResendLine = 0;
    uint32_t lastResponseTime = 0;

    // ADVANCED_OK
    bool advancedOK = false;
    uint16_t bufferFree = 0;
    uint16_t plannerFree = 0;

    // Process one complete response line
    void processResponse(char *line);
    void parseAdvancedOK(char *response);
    void readTemp(char *temp, size_t length, char *p);
    void readTemps(char *line);
    uint16_t effectiveWindow(void) const;
    void checkTimeout(void);
};


/* --- GCODE SENDER --- */

// Reads a gcode file from the SD card line by line
// Processes each line into a command to send
// Keeps a history of commands sent to recover from
// communications errors
//
// Basic usage:
// loadFile(FsFile *file);
// start(), pause(), resume(), stop() as commanded by the user
//
// Marlin class will call resend(line) as needed to correct errors

class GcodeSender
{
public:

    GcodeSender(Marlin &marlinRef)
        : marlin(marlinRef)
    {
        clearHistory();
    }

    // UPDATE
    // Call repeatedly from main:loop().
    void update()
    {
        if (!running) return;
        if (paused) return;
        if (finished) return;
        if (error) return;

        // See if printer is ready for another command
        if (!marlin.canSend()) return;

        sendNextCommand();
    }

    void loadFile(FsFile *fileptr) { file = fileptr; }

    // START
    bool start()
    {
        if (file == nullptr)
        {
            LOG("G_Sender: No G-code file");
            return false;
        }

        if (!file->seekSet(0))
        {
            LOG("G_Sender: Cannot seek to beginning");
            return false;
        }

        currentLine = 0;
        running = true;
        paused = false;
        finished = false;
        error = false;

        clearHistory();

        // Reset the serial comms controller
        marlin.resetState();

        LOG();
        LOG("=== GCODE START ===");

        return true;
    }

    // PAUSE
    void pause()
    {
        if (!running) return;

        paused = true;
        LOG("=== GCODE PAUSED ===");
    }

    // RESUME
    void resume()
    {
        if (!running) return;

        paused = false;
        LOG("=== GCODE RESUMED ===");
    }

    // STOP
    void stop()
    {
        running = false;
        paused = false;
        finished = false;
        currentLine = 0;

        clearHistory();
        marlin.resetState();

        LOG("=== GCODE STOPPED ===");
    }

    // RESEND
    // Called by Marlin when it receives a resend: request
    bool resend(uint32_t requestedLine)
    {
        uint64_t position;

        if (!running)
        {
            LOG("Resend received while G_Sender is stopped");
            return false;
        }

        LOGN("=== RESEND REQUEST: ");
        LOGN(requestedLine);
        LOG(" ===");

        // Find requested line in file-position history
        if (!findHistory( requestedLine, position))
        {
            LOGN("G_Send: Line ");
            LOGN(requestedLine);
            LOG(" not in history");

            error = true;
            return false;
        }

        LOGN("G_Send: Seeking to file position ");
        LOG(position);

        // Seek back to the first byte of that command
        if (!file->seekSet(position)) {
          LOG("G_Send: file->seekSet() failed");

          error = true;
          return false;
        }

        // currentLine is the line immediately BEFORE the
        // requested line because sendNextCommand() increments
        // it before constructing the next command.
        currentLine = requestedLine - 1;

        // Let Marlin fix up its command queue
        marlin.resendStarted();
        return true;
    }

    // STATUS
    bool isRunning() const { return running; }
    bool isPaused() const { return paused; }
    bool isFinished() const { return finished; }
    bool hasError() const { return error; }
    uint32_t getCurrentLine() const { return currentLine; }

private:

    FsFile *file = nullptr;
    Marlin &marlin;

    // STATE
    bool running = false;
    bool paused = false;
    bool finished = false;
    bool error = false;

    uint32_t currentLine = 0;

    // FILE POSITION HISTORY
    struct HistoryEntry
    {
        uint32_t line;
        uint64_t position;
        bool valid;
    };

    HistoryEntry history[MARLIN_HISTORY_SIZE];

    // CLEAR HISTORY
    void clearHistory()
    {
        for (int i = 0; i < MARLIN_HISTORY_SIZE; i++)
        {
            history[i].line = 0;
            history[i].position = 0;
            history[i].valid = false;
        }
    }

    // SAVE HISTORY
    void saveHistory(uint32_t line, uint64_t position)
    {
        uint32_t index = line % MARLIN_HISTORY_SIZE;

        history[index].line = line;
        history[index].position = position;
        history[index].valid = true;
    }

    // FIND HISTORY ENTRY
    bool findHistory(uint32_t line, uint64_t &position)
    {
        uint32_t index = line % MARLIN_HISTORY_SIZE;

        if (!history[index].valid) return false;
        if (history[index].line != line) return false;

        position = history[index].position;
        return true;
    }

    // Read a line from the gcode file
    bool readLine(
        char *buffer,
        size_t bufferSize,
        uint64_t &filePosition
    )
    {
        // Save the file position BEFORE reading the first byte.
        // to store in the history ring buffer
        filePosition = file->curPosition();

        size_t length = 0;
        bool gotData = false;

        while (file->available())
        {
            int c = file->read();

            if (c < 0) break;

            gotData = true;

            // Newline
            if (c == '\n') break;

            // Ignore carriage return
            if (c == '\r') continue;

            // Store character
            if (length < bufferSize - 1)
            {
                buffer[length++] = (char)c;
            }
            else
            {
                // Command doesn't fit into buffer
                // Consume the rest of the line
                while (file->available())
                {
                    c = file->read();
                    if (c == '\n') break;
                }

                buffer[0] = '\0';
                error = true;
                LOG("G_Send: G-code line too long");

                return false;
            }
        }

        buffer[length] = '\0';

        return gotData;
    }

    // REMOVE COMMENTS
    // Handles:
    //   ; comment
    //   (comment)
    void removeComments(char *line)
    {
        bool inParentheses = false;
        char *src = line;
        char *dst = line;

        while (*src)
        {
            if (*src == '(')
            {
                inParentheses = true;
                src++;
                continue;
            }

            if (*src == ')')
            {
                inParentheses = false;
                src++;
                continue;
            }

            if (!inParentheses && *src == ';')
                break;

            if (!inParentheses)
                *dst++ = *src;

            src++;
        }
        *dst = '\0';
    }

    // TRIM leading and trailing whitespace
    void trim(char *line)
    {
        char *start = line;

        // Leading whitespace
        while (*start == ' ' || *start == '\t' )
        {
            start++;
        }

        if (start != line)
        {
            memmove( line, start, strlen(start) + 1);
        }

        // Trailing whitespace
        size_t length = strlen(line);

        while (length > 0)
        {
            char c = line[length - 1];

            if (c == ' ' || c == '\t')
            {
                line[--length] = '\0';
            }
            else
            {
                break;
            }
        }
    }

    // READ NEXT GCODE COMMAND
    // Blank lines and comment-only lines are skipped.
    //
    // filePosition always refers to the first byte of the
    // file containing the command
    bool readNextCommand(
        char *command,
        size_t commandSize,
        uint64_t &filePosition
    )
    {
        char line[MARLIN_TX_LENGTH];

        while (true)
        {
            if (!readLine(line, sizeof(line), filePosition))
            {
                return false;
            }

            removeComments(line);
            trim(line);

            // Ignore blank or comment-only lines
            if (line[0] == '\0') continue;

            strncpy(command, line, commandSize - 1);
            command[commandSize - 1] = '\0';
            return true;
        }
    }

    // MARLIN XOR CHECKSUM
    //
    // Checksum is calculated over:
    //     N123 G1 X10 Y20
    // and does NOT include:
    //     *checksum
    uint8_t calculateChecksum(const char *buffer)
    {
        uint8_t checksum = 0;

        while (*buffer)
            checksum ^= (uint8_t)*buffer++;

        return checksum;
    }

    // BUILD MARLIN COMMAND
    bool buildMarlinCommand(const char *gcode, char *buffer, size_t bufferSize)
    {
        int length =
            snprintf(
                buffer,
                bufferSize,
                "N%lu %s",
                (unsigned long)currentLine,
                gcode
            );

        if (length < 0 || (size_t)length >= bufferSize)
        {
            return false;
        }


        uint8_t checksum = calculateChecksum(buffer);
        int remaining = bufferSize - length;

        int added =
            snprintf(
                buffer + length,
                remaining,
                "*%u",
                checksum
            );

        if (added < 0 || added >= remaining)
            return false;

        return true;
    }

    // SEND NEXT COMMAND
    void sendNextCommand()
    {
        char gcode[MARLIN_TX_LENGTH];
        char ncode[MARLIN_TX_LENGTH];
        uint64_t filePosition;

        // Read next gcode command
        if (!readNextCommand(gcode, sizeof(gcode), filePosition))
        {
            if (!error)
            {
                finished = true;
                running = false;

                LOG();
                LOG("=== GCODE COMPLETE ===");
            }
            return;
        }

        // Assign the next line number to the command
        currentLine++;

        // Save the file position to history
        saveHistory(currentLine, filePosition);

        // Convert the command to the N<number><command>*<checksum> format
        if (!buildMarlinCommand(gcode, ncode, sizeof(ncode)))
        {
            LOG("G_Send: Cannot construct Marlin command");
            error = true;
            running = false;
            return;
        }

        // Send completed command to Marlin.
        marlin.send(ncode);
    }
};

#endif // MARLIN_H
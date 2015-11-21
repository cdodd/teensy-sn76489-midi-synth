// ============================ Global Variables ==============================

// Constants for referring to the available voices
const byte square1 = 0;
const byte square2 = 1;
const byte square3 = 2;
const byte noise = 3;

// Specify the write enable ("we") pin number
const byte wePin = 21;

// Specify the speed of the oscillator (in Hz)
const long clockHz = 1843200;

// Arrays for storing the following for each voice:
//    - Attenuation register (const)
//    - Current note
//    - Current pitchbend (excluding noise voice)
//    - Current velocity
//    - Frequency register (const) (excluding noise voice)
//    - LED pin number (const)
//
// In these arrays:
//    Index 0 = Square 1
//    Index 1 = Square 2
//    Index 2 = Square 3
//    Index 3 = Noise
const byte attenuationRegister[4] = {0x10, 0x30, 0x50, 0x70};
const byte frequencyRegister[3] = {0x00, 0x20, 0x40};
const byte ledPin[4] = {5, 9, 10, 12};
byte currentNote[4] = {0, 0, 0, 0};
byte currentVelocity[4] = {0, 0, 0, 0};
int currentPitchBend[3] = {8192, 8192, 8192};

// ============================ Arduino Functions =============================

void setup() {
    // Configure the PORTB pins as OUTPUTs and set them to LOW
    DDRB = 0xff;
    PORTB = 0x00;

    // Configure the "we" pin as an OUTPUT and set it to HIGH
    pinMode(wePin, OUTPUT);
    digitalWrite(wePin, HIGH);

    // Loop over each voice and...
    for (int i = 0; i <= 3; i++) {
        // Silence the voice (velocity value for each voice has already been
        // set to 0 in the currentVelocity array
        updateAttenuation(i);

        // Configure the voice LED pin as an OUTPUT and turn it off
        pinMode(ledPin[i], OUTPUT);
        analogWrite(ledPin[i], 0);
    }

    // Set up the MIDI callback functions
    usbMIDI.setHandleControlChange(onControlChange);
    usbMIDI.setHandleNoteOff(onNoteOff);
    usbMIDI.setHandleNoteOn(onNoteOn);
    usbMIDI.setHandlePitchChange(onPitchChange);
}

void loop() {
    usbMIDI.read();
}

// ========================= MIDI Callback Functions ==========================

void onControlChange(byte channel, byte control, byte value) {
    byte voice = channel - 1;

    // Mod wheel on noise voice. This control varies the frequency of square
    // tone 3 while using the noise voice. Notes F4 and C5 use the frequency
    // of square voice 3 for the noise shift rate.
    // NOTE: This control will interfere with any notes using square voice 3 on
    // MIDI channel 3.
    if (voice == noise && control == 1) {
        // Invert and shift the 7-bit CC value (0 -> 127) into a 10-bit value
        // (1023 -> 0) and use it as the frequency for square voice 3
        //
        // When the CC value is 127 the frequency value will be 0 which causes
        // the SN76489 to wrap around from the high frequency back to the low.
        // 1 is added to the value to prevent this happening.
        setSquareFrequency(square3, ((127 - value) << 3) + 1);
    }
}

void onNoteOff(byte channel, byte note, byte velocity) {
    byte voice = channel - 1;

    // Return if an unsupported voice number is given
    if (voice < 0 || voice > 3) {
        return;
    }

    // Check if this function is called for a note that is not the current
    // note. If it's not the current note then do nothing. This can happen when
    // a note is pressed, then another, then the original note is released but
    // the second note is still depressed.
    if (note != currentNote[voice]) {
        return;
    }

    // Set the velocity of the given voice to 0 and update the attenuation
    currentVelocity[voice] = 0;
    updateAttenuation(voice);
}

void onNoteOn(byte channel, byte note, byte velocity) {
    boolean updateAttenuationFlag;
    byte voice = channel - 1;

    // Return if an unsupported voice number is given
    if (voice < 0 || voice > 3) {
        return;
    }

    // Save the note and velocity for the given voice
    currentNote[voice] = note;
    currentVelocity[voice] = velocity;

    // Update the noise control/square frequency for the given voice
    if (voice == noise) {
        updateAttenuationFlag = updateNoiseControl();
    }
    else {
        updateAttenuationFlag = updateSquarePitch(voice);
    }

    // If updateNoiseControl or updateSquarePitch returned true then update the
    // attenuation. The update functions return false if an out of range note
    // was triggered. In this case no sound should be emitted.
    if (updateAttenuationFlag == true) {
        updateAttenuation(voice);
    }
}

void onPitchChange(byte channel, int pitch) {
    byte voice = channel - 1;

    // Return if an unsupported voice number is given. Note that the noise
    // voice is not supported here.
    if (voice < 0 || voice > 2) {
        return;
    }

    // Save the pitchbend for the given voice
    currentPitchBend[voice] = pitch;

    // Update the pitch for the given voice
    updateSquarePitch(voice);
}

// =========================== Auxiliary  Functions ===========================

void sendByte(byte data) {
    // Put the byte on the data pins
    PORTB = data;

    // Drop the "we" pin to start strobing the data onto the SN76489
    digitalWrite(wePin, LOW);

    // Delay for a short time while the data is read
    delayMicroseconds(50);

    // Raise the "we" pin to finish writing the data
    digitalWrite(wePin, HIGH);
}

void setSquareFrequency(byte voice, int frequencyData) {
    // Return if an unsupported voice number is given. Note that the noise
    // voice is not supported here.
    if (voice < 0 || voice > 2) {
        return;
    }

    // Send the two byte frequency data to the SN76489
    sendByte(0x80 | frequencyRegister[voice] | (frequencyData & 0x0f));
    sendByte(frequencyData >> 4);
}

void updateAttenuation(byte voice) {
    byte attenuationValue;

    // Return if an unsupported voice number is given
    if (voice < 0 || voice > 3) {
        return;
    }

    // Invert and shift the voice's current 7-bit velocity (0 -> 127) into a
    // 4-bit value (15 -> 0)
    attenuationValue = (127 - currentVelocity[voice]) >> 3;

    // Send the attenuation data byte to the SN76489
    sendByte(0x80 | attenuationRegister[voice] | attenuationValue);

    // Update the brightness of the LED for the given voice. Shift the 7-bit
    // velocity value into a 4-bit value to match the 16 levels of attenuation,
    // then shift the value into an 8-bit value to be used with analogWrite.
    analogWrite(ledPin[voice], (currentVelocity[voice] >> 3) << 4);
}

boolean updateNoiseControl() {
    byte noiseControlData;

    switch (currentNote[noise]) {
        case 60:
            // Note: C4, Periodic noise, shift rate = clock speed (Hz) / 512
            noiseControlData = 0x00;
            break;

        case 62:
            // Note: D4, Periodic noise, shift rate = clock speed (Hz) / 1024
            noiseControlData = 0x01;
            break;

        case 64:
            // Note: E4, Periodic noise, shift rate = clock speed (Hz) / 2048
            noiseControlData = 0x02;
            break;

        case 65:
            // Note: F4, Perioic noise, shift rate = Square voice 3 frequency
            noiseControlData = 0x03;
            break;

        case 67:
            // Note: G4, White noise, shift rate = clock speed (Hz) / 512
            noiseControlData = 0x04;
            break;

        case 69:
            // Note: A4, White noise, shift rate = clock speed (Hz) / 1024
            noiseControlData = 0x05;
            break;

        case 71:
            // Note: B4, White noise, shift rate = clock speed (Hz) / 2048
            noiseControlData = 0x06;
            break;

        case 72:
            // Note: C5, White noise, shift rate = Square voice 3 frequency
            noiseControlData = 0x07;
            break;

        default:
            return false;
    }

    // Send the noise control byte to the SN76489 and return true
    sendByte(0x80 | 0x60 | noiseControlData);
    return true;
}

boolean updateSquarePitch(byte voice) {
    float pitchInHz;
    unsigned int frequencyData;

    // Return if an unsupported voice number is given. Note that the noise
    // voice is not supported here.
    if (voice < 0 || voice > 2) {
        return false;
    }

    // Derive a value for the tone frequency (frequencyData) based on the MIDI
    // note, pitch bend and clock speed values. Calculation of the pitch in Hz
    // from the MIDI note and pitch bend value was based on comments from:
    // http://dsp.stackexchange.com/questions/1645/converting-a-pitch-bend-midi-value-to-a-normal-pitch-value
    pitchInHz = 440 * pow(2, (float(currentNote[voice] - 69) / 12) +
        (float(currentPitchBend[voice] - 8192) / ((unsigned int)4096 * 12)));
    frequencyData = clockHz / float(32 * pitchInHz);

    // If frequencyData is out of range for the 10-bit value (0 -> 1023) the
    // SN76489 accepts then don't do anything. We don't need to check if
    // frequencyData < 0 as it's an unsigned int. The boundaries of
    // frequencyData vary depending on the speed of the clock.
    if (frequencyData > 1023) {
        return false;
    }

    // Finally, set the frequency of the voice and return true
    setSquareFrequency(voice, frequencyData);
    return true;
}

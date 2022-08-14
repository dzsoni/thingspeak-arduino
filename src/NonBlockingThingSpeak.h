/*
  ThingSpeak(TM) Communication Library For Arduino, ESP8266 and ESP32

  Enables an Arduino or other compatible hardware to write or read data to or from ThingSpeak,
  an open data platform for the Internet of Things with MATLAB analytics and visualization.

  ThingSpeak ( https://www.thingspeak.com ) is an analytic IoT platform service that allows you to aggregate, visualize and
  analyze live data streams in the cloud.

  Copyright 2020, The MathWorks, Inc.

  See the accompaning licence file for licensing information.
*/

// #define PRINT_DEBUG_MESSAGES
// #define PRINT_HTTP
#include <stack>
#include <functional>

#ifndef ThingSpeak_h
#define ThingSpeak_h

#define TS_VER "2.0.0"

#include "Arduino.h"
#include <Client.h>

#define THINGSPEAK_URL "api.thingspeak.com"
#define THINGSPEAK_PORT_NUMBER 80
#define THINGSPEAK_HTTPS_PORT_NUMBER 443

#ifdef ARDUINO_ARCH_AVR
#ifdef ARDUINO_AVR_YUN
#define TS_USER_AGENT "tslib-arduino/" TS_VER " (arduino yun)"
#else
#define TS_USER_AGENT "tslib-arduino/" TS_VER " (arduino uno or mega)"
#endif
#elif defined(ARDUINO_ARCH_ESP8266)
#define TS_USER_AGENT "tslib-arduino/" TS_VER " (ESP8266)"
#elif defined(ARDUINO_SAMD_MKR1000)
#define TS_USER_AGENT "tslib-arduino/" TS_VER " (arduino mkr1000)"
#elif defined(ARDUINO_SAM_DUE)
#define TS_USER_AGENT "tslib-arduino/" TS_VER " (arduino due)"
#elif defined(ARDUINO_ARCH_SAMD)
#define TS_USER_AGENT "tslib-arduino/" TS_VER " (arduino samd)"
#elif defined(ARDUINO_ARCH_SAM)
#define TS_USER_AGENT "tslib-arduino/" TS_VER " (arduino sam)"
#elif defined(ARDUINO_ARCH_SAMD_BETA)
#define TS_USER_AGENT "tslib-arduino/" TS_VER " (arduino samd_beta )"
#elif defined(ARDUINO_ARCH_ESP32)
#define TS_USER_AGENT "tslib-arduino/" TS_VER " (ESP32)"
#elif defined(ARDUINO_ARCH_SAMD_BETA)
#define TS_USER_AGENT "tslib-arduino/" TS_VER " (arduino vidor)"
#else
#define TS_USER_AGENT "tslib-arduino/" TS_VER " (unknown)"
#endif

#define FIELDNUM_MIN 1
#define FIELDNUM_MAX 8
#define FIELDLENGTH_MAX 255 // Max length for a field in ThingSpeak is 255 bytes (UTF-8)

#define TIMEOUT_MS_SERVERRESPONSE 5000 // Wait up to five seconds for server to respond

#define TS_OK_SUCCESS 200               // OK / Success
#define TS_ERR_BADAPIKEY 400            // Incorrect API key (or invalid ThingSpeak server address)
#define TS_ERR_BADURL 404               // Incorrect API key (or invalid ThingSpeak server address)
#define TS_ERR_OUT_OF_RANGE -101        // Value is out of range or string is too long (> 255 bytes)
#define TS_ERR_INVALID_FIELD_NUM -201   // Invalid field number specified
#define TS_ERR_SETFIELD_NOT_CALLED -210 // setField() was not called before writeFields()
#define TS_ERR_CONNECT_FAILED -301      // Failed to connect to ThingSpeak
#define TS_ERR_UNEXPECTED_FAIL -302     // Unexpected failure during write to ThingSpeak
#define TS_ERR_BAD_RESPONSE -303        // Unable to parse response
#define TS_ERR_TIMEOUT -304             // Timeout waiting for server to respond
#define TS_ERR_NOT_INSERTED -401        // Point was not inserted (most probable cause is the rate limit of once every 15 seconds)

// variables to store the values from the readMultipleFields functionality
#ifndef ARDUINO_AVR_UNO
typedef struct feedRecord
{
    String nextReadField[8];
    String nextReadStatus;
    String nextReadLatitude;
    String nextReadLongitude;
    String nextReadElevation;
    String nextReadCreatedAt;
} feed;
#endif

// Enables an Arduino, ESP8266, ESP32 or other compatible hardware to write or read data to or from ThingSpeak, an open data platform for the Internet of Things with MATLAB analytics and visualization.
class NonBlockingThingSpeakClass
{
public:
    NonBlockingThingSpeakClass()
    {
        resetWriteFields();
        this->lastReadStatus = TS_OK_SUCCESS;
    }

    /*
    Function: begin

    Summary:
    Initializes the ThingSpeak library and network settings using the ThingSpeak.com service.

    Parameters:
    client - EthernetClient, YunClient, TCPClient, or WiFiClient for HTTP connection and WiFiSSLClient, WiFiClientSecure for HTTPS connection created earlier in the sketch.

    Returns:
    Always returns true

    Notes:
    This does not validate the information passed in, or generate any calls to ThingSpeak.
    Include this ThingSpeak header file after including the client header file and the TS_ENABLE_SSL macro in the user sketch.
    */
    bool begin(Client &client)
    {
#ifdef PRINT_DEBUG_MESSAGES
        Serial.println("ts::tsBegin");
#endif

        this->setClient(&client);

        this->setPort(THINGSPEAK_PORT_NUMBER);
#if defined(TS_ENABLE_SSL)
#if defined(WIFISSLCLIENT_H) || defined(wificlientbearssl_h) || defined(WiFiClientSecure_h)
        this->setPort(THINGSPEAK_HTTPS_PORT_NUMBER);
#else
        Serial.println("WARNING: This library doesn't support SSL connection to ThingSpeak. Default HTTP Connection used.");
#endif
#endif

        resetWriteFields();
        this->lastReadStatus = TS_OK_SUCCESS;
        return true;
    }

    /*
    Function: writeField

    Summary:
    Write an integer value to a single field in a ThingSpeak channel

    Parameters:
    channelNumber - Channel number
    field - Field number (1-8) within the channel to write to.
    value - Integer value (from -32,768 to 32,767) to write.
    writeAPIKey - Write API key associated with the channel.  *If you share code with others, do _not_ share this key*

    Returns:
    HTTP status code of 200 if successful.

    Notes:
    See getLastReadStatus() for other possible return values.
    */
    void writeField(unsigned long channelNumber, unsigned int field, int value, const char *writeAPIKey)
    {
        char valueString[10]; // int range is -32768 to 32768, so 7 bytes including terminator, plus a little extra
        itoa(value, valueString, 10);
        writeField(channelNumber, field, valueString, writeAPIKey);
    }

    /*
    Function: writeField

    Summary:
    Write a long value to a single field in a ThingSpeak channel

    Parameters:
    channelNumber - Channel number
    field - Field number (1-8) within the channel to write to.
    value - Long value (from -2,147,483,648 to 2,147,483,647) to write.
    writeAPIKey - Write API key associated with the channel.  *If you share code with others, do _not_ share this key*

    Returns:
    HTTP status code of 200 if successful.

    Notes:
    See getLastReadStatus() for other possible return values.
    */
    void writeField(unsigned long channelNumber, unsigned int field, long value, const char *writeAPIKey)
    {
        char valueString[15]; // long range is -2147483648 to 2147483647, so 12 bytes including terminator
        ltoa(value, valueString, 10);
        writeField(channelNumber, field, valueString, writeAPIKey);
    }

    /*
    Function: writeField

    Summary:
    Write a floating point value to a single field in a ThingSpeak channel

    Parameters:
    channelNumber - Channel number
    field - Field number (1-8) within the channel to write to.
    value - Floating point value (from -999999000000 to 999999000000) to write.  If you need more accuracy, or a wider range, you should format the number using <tt>dtostrf</tt> and writeField().
    writeAPIKey - Write API key associated with the channel.  *If you share code with others, do _not_ share this key*

    Returns:
    HTTP status code of 200 if successful.

    Notes:
    See getLastReadStatus() for other possible return values.
    */
    void writeField(unsigned long channelNumber, unsigned int field, float value, const char *writeAPIKey)
    {
#ifdef PRINT_DEBUG_MESSAGES
        Serial.print("ts::writeField (channelNumber: ");
        Serial.print(channelNumber);
        Serial.print(" writeAPIKey: ");
        Serial.print(writeAPIKey);
        Serial.print(" field: ");
        Serial.print(field);
        Serial.print(" value: ");
        Serial.print(value, 5);
        Serial.println(")");
#endif
        char valueString[20]; // range is -999999000000.00000 to 999999000000.00000, so 19 + 1 for the terminator
        int status = convertFloatToChar(value, valueString);
        if (status != TS_OK_SUCCESS)
        {
            onWriteFieldCallback(status);
            return;
        }
        writeField(channelNumber, field, valueString, writeAPIKey);
    }

    /*
    Function: writeField

    Summary:
    Write a string to a single field in a ThingSpeak channel

    Parameters:
    channelNumber - Channel number
    field - Field number (1-8) within the channel to write to.
    value - String to write (UTF8 string).  ThingSpeak limits this field to 255 bytes.
    writeAPIKey - Write API key associated with the channel.  *If you share code with others, do _not_ share this key*

    Returns:
    HTTP status code of 200 if successful.

    Notes:
    See getLastReadStatus() for other possible return values.
    */
    void writeField(unsigned long channelNumber, unsigned int field, String value, const char *writeAPIKey)
    {
        // Invalid field number specified
        if (field < FIELDNUM_MIN || field > FIELDNUM_MAX)
        {
            onWriteFieldCallback(TS_ERR_INVALID_FIELD_NUM);
            return;
        }
        // Max # bytes for ThingSpeak field is 255
        if (value.length() > FIELDLENGTH_MAX)
        {
            onWriteFieldCallback(TS_ERR_OUT_OF_RANGE);
            return;
        }

#ifdef PRINT_DEBUG_MESSAGES
        Serial.print("ts::writeField (channelNumber: ");
        Serial.print(channelNumber);
        Serial.print(" writeAPIKey: ");
        Serial.print(writeAPIKey);
        Serial.print(" field: ");
        Serial.print(field);
        Serial.print(" value: \"");
        Serial.print(value);
        Serial.println("\")");
#endif
        String postMessage = String("field");
        postMessage.concat(field);
        postMessage.concat("=");
        postMessage.concat(value);

        stackofreturns.emplace([this](){this->onWriteFieldCallback(this->getLastReadStatus());});
        writeRawSilent(channelNumber, postMessage, writeAPIKey,true);
    }

    /*
    Function: setField

    Summary:
    Set the value of a single field that will be part of a multi-field update.

    Parameters:
    field - Field number (1-8) within the channel to set.
    value - Integer value (from -32,768 to 32,767) to set.

    Returns:
    Code of 200 if successful.
    Code of -101 if value is out of range or string is too long (> 255 bytes)
    */
    int setField(unsigned int field, int value)
    {
        char valueString[10]; // int range is -32768 to 32768, so 7 bytes including terminator
        itoa(value, valueString, 10);

        return setField(field, valueString);
    }

    /*
    Function: setField

    Summary:
    Set the value of a single field that will be part of a multi-field update.

    Parameters:
    field - Field number (1-8) within the channel to set.
    value - Long value (from -2,147,483,648 to 2,147,483,647) to write.

    Returns:
    Code of 200 if successful.
    Code of -101 if value is out of range or string is too long (> 255 bytes)
    */
    int setField(unsigned int field, long value)
    {
        char valueString[15]; // long range is -2147483648 to 2147483647, so 12 bytes including terminator
        ltoa(value, valueString, 10);

        return setField(field, valueString);
    }

    /*
    Function: setField

    Summary:
    Set the value of a single field that will be part of a multi-field update.

    Parameters:
    field - Field number (1-8) within the channel to set.
    value - Floating point value (from -999999000000 to 999999000000) to write.  If you need more accuracy, or a wider range, you should format the number yourself (using <tt>dtostrf</tt>) and setField() using the resulting string.

    Returns:
    Code of 200 if successful.
    Code of -101 if value is out of range or string is too long (> 255 bytes)
    */
    int setField(unsigned int field, float value)
    {
        char valueString[20]; // range is -999999000000.00000 to 999999000000.00000, so 19 + 1 for the terminator
        int status = convertFloatToChar(value, valueString);
        if (status != TS_OK_SUCCESS)
            return status;

        return setField(field, valueString);
    }

    /*
    Function: setField

    Summary:
    Set the value of a single field that will be part of a multi-field update.

    Parameters:
    field - Field number (1-8) within the channel to set.
    value - String to write (UTF8).  ThingSpeak limits this to 255 bytes.

    Returns:
    Code of 200 if successful.
    Code of -101 if value is out of range or string is too long (> 255 bytes)
    */
    int setField(unsigned int field, String value)
    {
#ifdef PRINT_DEBUG_MESSAGES
        Serial.print("ts::setField   (field: ");
        Serial.print(field);
        Serial.print(" value: \"");
        Serial.print(value);
        Serial.println("\")");
#endif
        if (field < FIELDNUM_MIN || field > FIELDNUM_MAX)
            return TS_ERR_INVALID_FIELD_NUM;
        // Max # bytes for ThingSpeak field is 255 (UTF-8)
        if (value.length() > FIELDLENGTH_MAX)
            return TS_ERR_OUT_OF_RANGE;
        this->nextWriteField[field - 1] = value;

        return TS_OK_SUCCESS;
    }

    /*
    Function: setLatitude

    Summary:
    Set the latitude of a multi-field update.

    Parameters:
    latitude - Latitude of the measurement as a floating point value (degrees N, use negative values for degrees S)

    Returns:
    Always return 200

    Notes:
    To record latitude, longitude and elevation of a write, call setField() for each of the fields you want to write. Then setLatitude(), setLongitude(), setElevation() and then call writeFields()
    */
    int setLatitude(float latitude)
    {
#ifdef PRINT_DEBUG_MESSAGES
        Serial.print("ts::setLatitude(latitude: ");
        Serial.print(latitude, 3);
        Serial.println("\")");
#endif
        this->nextWriteLatitude = latitude;

        return TS_OK_SUCCESS;
    }

    /*
    Function: setLongitude

    Summary:
    Set the longitude of a multi-field update.

    Parameters:
    longitude - Longitude of the measurement as a floating point value (degrees E, use negative values for degrees W)

    Returns:
    Always return 200

    Notes:
    To record latitude, longitude and elevation of a write, call setField() for each of the fields you want to write. Then setLatitude(), setLongitude(), setElevation() and then call writeFields()
    */
    int setLongitude(float longitude)
    {
#ifdef PRINT_DEBUG_MESSAGES
        Serial.print("ts::setLongitude(longitude: ");
        Serial.print(longitude, 3);
        Serial.println("\")");
#endif
        this->nextWriteLongitude = longitude;

        return TS_OK_SUCCESS;
    }

    /*
    Function: setElevation

    Summary:
    Set the elevation of a multi-field update.

    Parameters:
    elevation - Elevation of the measurement as a floating point value (meters above sea level)

    Returns:
    Always return 200

    Notes:
    To record latitude, longitude and elevation of a write, call setField() for each of the fields you want to write. Then setLatitude(), setLongitude(), setElevation() and then call writeFields()

    */
    int setElevation(float elevation)
    {
#ifdef PRINT_DEBUG_MESSAGES
        Serial.print("ts::setElevation(elevation: ");
        Serial.print(elevation, 3);
        Serial.println("\")");
#endif
        this->nextWriteElevation = elevation;

        return TS_OK_SUCCESS;
    }

    /*
    Function: setStatus

    Summary:
    Set the status field of a multi-field update.

    Parameters:
    status - String to write (UTF8).  ThingSpeak limits this to 255 bytes.

    Returns:
    Code of 200 if successful.
    Code of -101 if string is too long (> 255 bytes)

    Notes:
    To record a status message on a write, call setStatus() then call writeFields().
    Use status to provide additonal details when writing a channel update.
    Additonally, status can be used by the ThingTweet App to send a message to Twitter.
    */
    int setStatus(String status)
    {
#ifdef PRINT_DEBUG_MESSAGES
        Serial.print("ts::setStatus(status: ");
        Serial.print(status);
        Serial.println("\")");
#endif
        // Max # bytes for ThingSpeak field is 255 (UTF-8)
        if (status.length() > FIELDLENGTH_MAX)
            return TS_ERR_OUT_OF_RANGE;
        this->nextWriteStatus = status;

        return TS_OK_SUCCESS;
    }

    /*
    Function: setTwitterTweet

    Summary:
    Set the Twitter account and message to use for an update to be tweeted.

    Parameters:
    twitter - Twitter account name as a String.
    tweet - Twitter message as a String (UTF-8) limited to 140 character.

    Returns:
    Code of 200 if successful.
    Code of -101 if string is too long (> 255 bytes)

    Notes:
    To send a message to twitter call setTwitterTweet() then call writeFields().
    Prior to using this feature, a twitter account must be linked to your ThingSpeak account. Do this by logging into ThingSpeak and going to Apps, then ThingTweet and clicking Link Twitter Account.
    */
    int setTwitterTweet(String twitter, String tweet)
    {
#ifdef PRINT_DEBUG_MESSAGES
        Serial.print("ts::setTwitterTweet(twitter: ");
        Serial.print(twitter);
        Serial.print(", tweet: ");
        Serial.print(tweet);
        Serial.println("\")");
#endif
        // Max # bytes for ThingSpeak field is 255 (UTF-8)
        if ((twitter.length() > FIELDLENGTH_MAX) || (tweet.length() > FIELDLENGTH_MAX))
            return TS_ERR_OUT_OF_RANGE;

        this->nextWriteTwitter = twitter;
        this->nextWriteTweet = tweet;

        return TS_OK_SUCCESS;
    }

    /*
    Function: setCreatedAt

    Summary:
    Set the created-at date of a multi-field update.

    Parameters:
    createdAt - Desired timestamp to be included with the channel update as a String.  The timestamp string must be in the ISO 8601 format. Example "2017-01-12 13:22:54"

    Returns:
    Code of 200 if successful.
    Code of -101 if string is too long (> 255 bytes)

    Notes:
    Timezones can be set using the timezone hour offset parameter. For example, a timestamp for Eastern Standard Time is: "2017-01-12 13:22:54-05".
    If no timezone hour offset parameter is used, UTC time is assumed.
    */
    int setCreatedAt(String createdAt)
    {
#ifdef PRINT_DEBUG_MESSAGES
        Serial.print("ts::setCreatedAt(createdAt: ");
        Serial.print(createdAt);
        Serial.println("\")");
#endif

        // the ISO 8601 format is too complicated to check for valid timestamps here
        // we'll need to reply on the api to tell us if there is a problem
        // Max # bytes for ThingSpeak field is 255 (UTF-8)
        if (createdAt.length() > FIELDLENGTH_MAX)
            return TS_ERR_OUT_OF_RANGE;
        this->nextWriteCreatedAt = createdAt;

        return TS_OK_SUCCESS;
    }

    /*
    Function: writeFields

    Summary:
    Write a multi-field update.

    Parameters:
    channelNumber - Channel number
    writeAPIKey - Write API key associated with the channel.  *If you share code with others, do _not_ share this key*

    Returns:
    200 - successful.
    404 - Incorrect API key (or invalid ThingSpeak server address)
    -101 - Value is out of range or string is too long (> 255 characters)
    -201 - Invalid field number specified
    -210 - setField() was not called before writeFields()
    -301 - Failed to connect to ThingSpeak
    -302 - Unexpected failure during write to ThingSpeak
    -303 - Unable to parse response
    -304 - Timeout waiting for server to respond
    -401 - Point was not inserted (most probable cause is the rate limit of once every 15 seconds)

    Notes:
    Call setField(), setLatitude(), setLongitude(), setElevation() and/or setStatus() and then call writeFields()
    */
    void writeFields(unsigned long channelNumber, const char *writeAPIKey)
    {
        if (!connectThingSpeak())
        {
            // Failed to connect to ThingSpeak
            onWriteFieldsCallback(TS_ERR_CONNECT_FAILED);
            return;
        }

        // Get the content length of the payload
        int contentLen = getWriteFieldsContentLength();

        if (contentLen == 0)
        {
            // setField was not called before writeFields
            onWriteFieldsCallback(TS_ERR_SETFIELD_NOT_CALLED);
            return;
        }

#ifdef PRINT_DEBUG_MESSAGES
        Serial.print("ts::writeFields   (channelNumber: ");
        Serial.print(channelNumber);
        Serial.print(" writeAPIKey: ");
        Serial.println(writeAPIKey);
#endif

        // Post data to thingspeak
        if (!this->client->print("POST /update HTTP/1.1\r\n"))
        {
            onWriteFieldsCallback(abortWriteRaw());
            return;
        }
        if (!writeHTTPHeader(writeAPIKey))
        {
            onWriteFieldsCallback(abortWriteRaw());
            return;
        }
        if (!this->client->print("Content-Type: application/x-www-form-urlencoded\r\n"))
        {
            onWriteFieldsCallback(abortWriteRaw());
            return;
        }
        if (!this->client->print("Content-Length: "))
        {
            onWriteFieldsCallback(abortWriteRaw());
            return;
        }
        if (!this->client->print(contentLen))
        {
            onWriteFieldsCallback(abortWriteRaw());
            return;
        }
        if (!this->client->print("\r\n\r\n"))
        {
            onWriteFieldsCallback(abortWriteRaw());
            return;
        }

        bool fFirstItem = true;
        for (size_t iField = 0; iField < FIELDNUM_MAX; iField++)
        {
            if (this->nextWriteField[iField].length() > 0)
            {
                if (!fFirstItem)
                {
                    if (!this->client->print("&"))
                    {
                        onWriteFieldsCallback(abortWriteRaw());
                        return;
                    }
                }
                if (!this->client->print("field"))
                {
                    onWriteFieldsCallback(abortWriteRaw());
                    return;
                }
                if (!this->client->print(iField + 1))
                {
                    onWriteFieldsCallback(abortWriteRaw());
                    return;
                }
                if (!this->client->print("="))
                {
                    onWriteFieldsCallback(abortWriteRaw());
                    return;
                }
                if (!this->client->print(this->nextWriteField[iField]))
                {
                    onWriteFieldsCallback(abortWriteRaw());
                    return;
                }
                fFirstItem = false;
            }
        }

        if (!isnan(this->nextWriteLatitude))
        {
            if (!fFirstItem)
            {
                if (!this->client->print("&"))
                {
                    onWriteFieldsCallback(abortWriteRaw());
                    return;
                }
            }
            if (!this->client->print("lat="))
            {
                onWriteFieldsCallback(abortWriteRaw());
                return;
            }
            if (!this->client->print(this->nextWriteLatitude))
            {
                onWriteFieldsCallback(abortWriteRaw());
                return;
            }
            fFirstItem = false;
        }

        if (!isnan(this->nextWriteLongitude))
        {
            if (!fFirstItem)
            {
                if (!this->client->print("&"))
                {
                    onWriteFieldsCallback(abortWriteRaw());
                    return;
                }
            }
            if (!this->client->print("long="))
            {
                onWriteFieldsCallback(abortWriteRaw());
                return;
            }
            if (!this->client->print(this->nextWriteLongitude))
            {
                onWriteFieldsCallback(abortWriteRaw());
                return;
            }
            fFirstItem = false;
        }

        if (!isnan(this->nextWriteElevation))
        {
            if (!fFirstItem)
            {
                if (!this->client->print("&"))
                {
                    onWriteFieldsCallback(abortWriteRaw());
                    return;
                }
            }
            if (!this->client->print("elevation="))
            {
                onWriteFieldsCallback(abortWriteRaw());
                return;
            }
            if (!this->client->print(this->nextWriteElevation))
            {
                onWriteFieldsCallback(abortWriteRaw());
                return;
            }
            fFirstItem = false;
        }

        if (this->nextWriteStatus.length() > 0)
        {
            if (!fFirstItem)
            {
                if (!this->client->print("&"))
                {
                    onWriteFieldsCallback(abortWriteRaw());
                    return;
                }
            }
            if (!this->client->print("status="))
            {
                onWriteFieldsCallback(abortWriteRaw());
                return;
            }
            if (!this->client->print(this->nextWriteStatus))
            {
                onWriteFieldsCallback(abortWriteRaw());
                return;
            }
            fFirstItem = false;
        }

        if (this->nextWriteTwitter.length() > 0)
        {
            if (!fFirstItem)
            {
                if (!this->client->print("&"))
                {
                    onWriteFieldsCallback(abortWriteRaw());
                    return;
                }
            }
            if (!this->client->print("twitter="))
            {
                onWriteFieldsCallback(abortWriteRaw());
                return;
            }
            if (!this->client->print(this->nextWriteTwitter))
            {
                onWriteFieldsCallback(abortWriteRaw());
                return;
            }
            fFirstItem = false;
        }

        if (this->nextWriteTweet.length() > 0)
        {
            if (!fFirstItem)
            {
                if (!this->client->print("&"))
                {
                    onWriteFieldsCallback(abortWriteRaw());
                    return;
                }
            }
            if (!this->client->print("tweet="))
            {
                onWriteFieldsCallback(abortWriteRaw());
                return;
            }
            if (!this->client->print(this->nextWriteTweet))
            {
                onWriteFieldsCallback(abortWriteRaw());
                return;
            }
            fFirstItem = false;
        }

        if (this->nextWriteCreatedAt.length() > 0)
        {
            if (!fFirstItem)
            {
                if (!this->client->print("&"))
                {
                    onWriteFieldsCallback(abortWriteRaw());
                    return;
                }
            }
            if (!this->client->print("created_at="))
            {
                onWriteFieldsCallback(abortWriteRaw());
                return;
            }
            if (!this->client->print(this->nextWriteCreatedAt))
            {
                onWriteFieldsCallback(abortWriteRaw());
                return;
            }
            fFirstItem = false;
        }

        if (!this->client->print("&headers=false"))
        {
            onWriteFieldsCallback(abortWriteRaw());
            return;
        }

        resetWriteFields();
        stackofreturns.emplace([this](){this->stackofreturns.pop();
                                    onWriteFieldsCallback(lastReadStatus);});
        finishWrite();
    }

    /*
    Function: writeRaw

    Summary:
    Write a raw POST to a ThingSpeak channel

    Parameters:
    channelNumber - Channel number
    postMessage - Raw URL to write to ThingSpeak as a string.  See the documentation at https://thingspeak.com/docs/channels#update_feed.
    writeAPIKey - Write API key associated with the channel.  *If you share code with others, do _not_ share this key*

    Returns:
    200 - successful.
    404 - Incorrect API key (or invalid ThingSpeak server address)
    -101 - Value is out of range or string is too long (> 255 characters)
    -201 - Invalid field number specified
    -210 - setField() was not called before writeFields()
    -301 - Failed to connect to ThingSpeak
    -302 - Unexpected failure during write to ThingSpeak
    -303 - Unable to parse response
    -304 - Timeout waiting for server to respond
    -401 - Point was not inserted (most probable cause is the rate limit of once every 15 seconds)

    Notes:
    This is low level functionality that will not be required by most users.
    */
    void writeRaw(unsigned long channelNumber, String postMessage, const char *writeAPIKey)
    {
        writeRawSilent(channelNumber,postMessage,writeAPIKey,false);
    }
    void writeRawSilent(unsigned long channelNumber, String postMessage, const char *writeAPIKey,bool silent=false)
    {
#ifdef PRINT_DEBUG_MESSAGES
        Serial.print("ts::writeRaw   (channelNumber: ");
        Serial.print(channelNumber);
        Serial.print(" writeAPIKey: ");
        Serial.println(writeAPIKey);
#endif

        if (!connectThingSpeak())
        {
            // Failed to connect to ThingSpeak
            lastReadStatus=TS_ERR_CONNECT_FAILED;
            if(!silent)onWriteRawCallback(TS_ERR_CONNECT_FAILED);
            return;
        }

        postMessage.concat("&headers=false");

#ifdef PRINT_DEBUG_MESSAGES
        Serial.print("               POST \"");
        Serial.print(postMessage);
        Serial.println("\"");
#endif

        // Post data to thingspeak
        if (!this->client->print("POST /update HTTP/1.1\r\n"))
        {   
            lastReadStatus=abortWriteRaw();
            if(!silent)onWriteRawCallback(lastReadStatus);
            return;
        }
        if (!writeHTTPHeader(writeAPIKey))
            {
            lastReadStatus=abortWriteRaw();
            if(!silent)onWriteRawCallback(lastReadStatus);
            return;
        }
        if (!this->client->print("Content-Type: application/x-www-form-urlencoded\r\n"))
            {
            lastReadStatus=abortWriteRaw();
            if(!silent)onWriteRawCallback(lastReadStatus);
            return;
        }
        if (!this->client->print("Content-Length: "))
            {
            lastReadStatus=abortWriteRaw();
            if(!silent)onWriteRawCallback(lastReadStatus);
            return;
        }
        if (!this->client->print(postMessage.length()))
            {
            lastReadStatus=abortWriteRaw();
            if(!silent)onWriteRawCallback(lastReadStatus);
            return;
        }
        if (!this->client->print("\r\n\r\n"))
            {
            lastReadStatus=abortWriteRaw();
            if(!silent)onWriteRawCallback(lastReadStatus);
            return;
        }
        if (!this->client->print(postMessage))
            {
            lastReadStatus=abortWriteRaw();
            if(!silent)onWriteRawCallback(lastReadStatus);
            return;
        }

        resetWriteFields();
        if(!silent)
        {
            stackofreturns.emplace([this](){this->stackofreturns.pop();
                                    onWriteRawCallback(getLastReadStatus());});
        }
        finishWrite();
    }

    /*
    Function: readStringField

    Summary:
    Read the latest string from a private ThingSpeak channel

    Parameters:
    channelNumber - Channel number
    field - Field number (1-8) within the channel to read from.
    readAPIKey - Read API key associated with the channel.  *If you share code with others, do _not_ share this key*

    Returns:
    Value read (UTF8 string), or empty string if there is an error.  Use getLastReadStatus() to get more specific information.
    */
    void readStringField(unsigned long channelNumber, unsigned int field, const char *readAPIKey)
    {
        readStringFieldSilent(channelNumber, field, readAPIKey, false);
    }

    void readStringFieldSilent(unsigned long channelNumber, unsigned int field, const char *readAPIKey, bool silent = false)
    {
        if (field < FIELDNUM_MIN || field > FIELDNUM_MAX)
        {
            this->lastReadStatus = TS_ERR_INVALID_FIELD_NUM;
            httpresponsetext = "";
            if (!silent)
                onReadStringFieldCallback(httpresponsetext);
            return;
        }
#ifdef PRINT_DEBUG_MESSAGES
        Serial.print("ts::readStringField(channelNumber: ");
        Serial.print(channelNumber);
        if (NULL != readAPIKey)
        {
            Serial.print(" readAPIKey: ");
            Serial.print(readAPIKey);
        }
        Serial.print(" field: ");
        Serial.print(field);
        Serial.println(")");
#endif
        String suffixURL = String("/fields/");
        suffixURL.concat(field);
        suffixURL.concat("/last");
        if (!silent)
        {
            stackofreturns.emplace([this]()
                                   {stackofreturns.pop(); onReadStringFieldCallback(httpresponsetext); });
        }
        readRaw(channelNumber, suffixURL, readAPIKey);
    }

    /*
    Function: readStringField

    Summary:
    Read the latest string from a public ThingSpeak channel

    Parameters:
    channelNumber - Channel number
    field - Field number (1-8) within the channel to read from.

    Returns:
    Value read (UTF8 string), or empty string if there is an error.  Use getLastReadStatus() to get more specific information.
    */
    void readStringField(unsigned long channelNumber, unsigned int field)
    {
        readStringField(channelNumber, field, NULL);
    }

    /*
    Function: readFloatField

    Summary:
    ead the latest floating point value from a private ThingSpeak channel

    Parameters:
    channelNumber - Channel number
    field - Field number (1-8) within the channel to read from.
    readAPIKey - Read API key associated with the channel.  *If you share code with others, do _not_ share this key*

    Returns:
    Value read, or 0 if the field is text or there is an error.  Use getLastReadStatus() to get more specific information.  Note that NAN, INFINITY, and -INFINITY are valid results.
    */
    void readFloatField(unsigned long channelNumber, unsigned int field, const char *readAPIKey)
    {
        stackofreturns.emplace([this]()
                               {
            this->stackofreturns.pop();
            onReadFloatFieldCallback(this->convertStringToFloat(this->httpresponsetext)); });
        readStringFieldSilent(channelNumber, field, readAPIKey, true);
    }

    /*
    Function: readFloatField

    Summary:
    Read the latest floating point value from a public ThingSpeak channel

    Parameters:
    channelNumber - Channel number
    field - Field number (1-8) within the channel to read from.

    Returns:
    Value read, or 0 if the field is text or there is an error.  Use getLastReadStatus() to get more specific information.  Note that NAN, INFINITY, and -INFINITY are valid results.
    */
    void readFloatField(unsigned long channelNumber, unsigned int field)
    {
        readFloatField(channelNumber, field, NULL);
    }

    /*
    Function: readLongField

    Summary:
    Read the latest long value from a private ThingSpeak channel

    Parameters:
    channelNumber - Channel number
    field - Field number (1-8) within the channel to read from.
    readAPIKey - Read API key associated with the channel.  *If you share code with others, do _not_ share this key*

    Returns:
    Value read, or 0 if the field is text or there is an error.  Use getLastReadStatus() to get more specific information.  Note that NAN, INFINITY, and -INFINITY are valid results.
    */
    void readLongField(unsigned long channelNumber, unsigned int field, const char *readAPIKey)
    {
        // Note that although the function is called "toInt" it really returns a long.
        stackofreturns.emplace([this]()
                               {
            this->stackofreturns.pop();
            onReadLongFieldCallback(this->httpresponsetext.toInt()); });
        readStringFieldSilent(channelNumber, field, readAPIKey, true);
    }

    /*
    Function: readLongField

    Summary:
    Read the latest long value from a public ThingSpeak channel

    Parameters:
    channelNumber - Channel number
    field - Field number (1-8) within the channel to read from.

    Returns:
    Value read, or 0 if the field is text or there is an error.  Use getLastReadStatus() to get more specific information.  Note that NAN, INFINITY, and -INFINITY are valid results.
    */
    void readLongField(unsigned long channelNumber, unsigned int field)
    {
        readLongField(channelNumber, field, NULL);
    }

    /*
    Function: readIntField

    Summary:
    Read the latest int value from a private ThingSpeak channel

    Parameters:
    channelNumber - Channel number
    field - Field number (1-8) within the channel to read from.
    readAPIKey - Read API key associated with the channel.  *If you share code with others, do _not_ share this key*

    Returns:
    Value read, or 0 if the field is text or there is an error.  Use getLastReadStatus() to get more specific information.  Note that NAN, INFINITY, and -INFINITY are valid results.
    */
    void readIntField(unsigned long channelNumber, unsigned int field, const char *readAPIKey)
    {
        stackofreturns.emplace([this]()
                               {
            this->stackofreturns.pop();
            onReadIntFieldCallback(this->httpresponsetext.toInt()); });
        readStringFieldSilent(channelNumber, field, readAPIKey, true);
    }

    /*
    Function: readIntField

    Summary:
    Read the latest int value from a public ThingSpeak channel

    Parameters:
    channelNumber - Channel number
    field - Field number (1-8) within the channel to read from.

    Returns:
    Value read, or 0 if the field is text or there is an error.  Use getLastReadStatus() to get more specific information.  Note that NAN, INFINITY, and -INFINITY are valid results.
    */
    void readIntField(unsigned long channelNumber, unsigned int field)
    {
        readIntField(channelNumber, field, NULL);
    }

    /*
    Function: readStatus

    Summary:
    Read the latest status from a private ThingSpeak channel

    Parameters:
    channelNumber - Channel number
    readAPIKey - Read API key associated with the channel.  *If you share code with others, do _not_ share this key*

    Results:
    Value read (UTF8 string). An empty string is returned if there was no status written to the channel or in case of an error.  Use getLastReadStatus() to get more specific information.
    */
    void readStatus(unsigned long channelNumber, const char *readAPIKey)
    {
        stackofreturns.emplace([this]()
                               { readStatus_1(); });
        readRaw(channelNumber, "/feeds/last.txt?status=true", readAPIKey);
    }
    void readStatus_1()
    {
        stackofreturns.pop();
        if (getLastReadStatus() != TS_OK_SUCCESS)
        {
            httpresponsetext = "";
            onReadStatusCallback(httpresponsetext);
            return;
        }
        onReadStatusCallback(getJSONValueByKey(httpresponsetext, "status"));
    }

    /*
    Function: readStatus

    Summary:
    Read the latest status from a public ThingSpeak channel

    Parameters:
    channelNumber - Channel number

    Results:
    Value read (UTF8 string). An empty string is returned if there was no status written to the channel or in case of an error.  Use getLastReadStatus() to get more specific information.
    */
    void readStatus(unsigned long channelNumber)
    {
        readStatus(channelNumber, NULL);
    }

    /*
    Function: readCreatedAt

    Summary:
    Read the created-at timestamp associated with the latest update to a private ThingSpeak channel

    Parameters:
    channelNumber - Channel number
    readAPIKey - Read API key associated with the channel.  *If you share code with others, do _not_ share this key*

    Results:
    Value read (UTF8 string). An empty string is returned if there was no created-at timestamp written to the channel or in case of an error.  Use getLastReadStatus() to get more specific information.
    */
    void readCreatedAt(unsigned long channelNumber, const char *readAPIKey)
    {
        stackofreturns.emplace([this]()
                               { readCreatedAt_1(); });
        readRaw(channelNumber, "/feeds/last.txt", readAPIKey);
    }
    void readCreatedAt_1()
    {
        stackofreturns.pop();
        if (getLastReadStatus() != TS_OK_SUCCESS)
        {
            httpresponsetext = "";
            onReadCreatedAtCallback(httpresponsetext);
            return;
        }

        onReadCreatedAtCallback(getJSONValueByKey(httpresponsetext, "created_at"));
    }

    /*
    Function: readCreatedAt

    Summary:
    Read the created-at timestamp associated with the latest update to a public ThingSpeak channel

    Parameters:
    channelNumber - Channel number

    Results:
    Value read (UTF8 string). An empty string is returned if there was no created-at timestamp written to the channel or in case of an error.  Use getLastReadStatus() to get more specific information.
    */
    void readCreatedAt(unsigned long channelNumber)
    {
        readCreatedAt(channelNumber, NULL);
    }

    /*
    Function: readRaw

    Summary:
    Read a raw response from a private ThingSpeak channel

    Parameters:
    channelNumber - Channel number
    suffixURL - Raw URL to write to ThingSpeak as a String.  See the documentation at https://thingspeak.com/docs/channels#get_feed
    readAPIKey - Read API key associated with the channel.  *If you share code with others, do _not_ share this key*

    Returns:
    Response if successful, or empty string. Use getLastReadStatus() to get more specific information.

    Notes:
    This is low level functionality that will not be required by most users.
    */
    void readRaw(unsigned long channelNumber, String suffixURL, const char *readAPIKey)
    {
#ifdef PRINT_DEBUG_MESSAGES
        Serial.print("ts::readRaw   (channelNumber: ");
        Serial.print(channelNumber);
        if (NULL != readAPIKey)
        {
            Serial.print(" readAPIKey: ");
            Serial.print(readAPIKey);
        }
        Serial.print(" suffixURL: \"");
        Serial.print(suffixURL);
        Serial.println("\")");
#endif

        if (!connectThingSpeak())
        {
            this->lastReadStatus = TS_ERR_CONNECT_FAILED;
            httpresponsetext = "";
            return;
        }

        String readURL = String("/channels/");
        readURL.concat(channelNumber);
        readURL.concat(suffixURL);

#ifdef PRINT_DEBUG_MESSAGES
        Serial.print("               GET \"");
        Serial.print(readURL);
        Serial.println("\"");
#endif

        // Get data from thingspeak
        if (!this->client->print("GET "))
        {
            httpresponsetext = abortReadRaw();
            return;
        }
        if (!this->client->print(readURL))
        {
            httpresponsetext = abortReadRaw();
            return;
        }
        if (!this->client->print(" HTTP/1.1\r\n"))
        {
            httpresponsetext = abortReadRaw();
            return;
        }
        if (!writeHTTPHeader(readAPIKey))
        {
            httpresponsetext = abortReadRaw();
            return;
        }
        if (!this->client->print("\r\n"))
        {
            httpresponsetext = abortReadRaw();
            return;
        }

        // make sure all of the HTTP request is pushed out of the buffer before looking for a response
        this->client->flush();
        timestamp1 = millis();
        stackofreturns.emplace([this]()
                               { this->readRaw_1(); });
        stackofreturns.emplace([this]()
                               { this->getHTTPResponse(); });
        getHTTPResponse();
    }

    void readRaw_1()
    {
        emptyStream();

#ifdef PRINT_DEBUG_MESSAGES
        if (status == TS_OK_SUCCESS)
        {
            Serial.print("Read: \"");
            Serial.print(content);
            Serial.println("\"");
        }
#endif

        this->client->stop();
#ifdef PRINT_DEBUG_MESSAGES
        Serial.println("disconnected.");
#endif

        if (lastReadStatus != TS_OK_SUCCESS)
        {
            httpresponsetext = "";
        }
        stackofreturns.pop();
        return;
    }

    /*
    Function: readRaw

    Summary:
    Read a raw response from a public ThingSpeak channel

    Parameters:
    channelNumber - Channel number
    suffixURL - Raw URL to write to ThingSpeak as a String.  See the documentation at https://thingspeak.com/docs/channels#get_feed

    Returns:
    Response if successful, or empty string. Use getLastReadStatus() to get more specific information.

    Notes:
    This is low level functionality that will not be required by most users.
    */
    void readRaw(unsigned long channelNumber, String suffixURL)
    {
        readRaw(channelNumber, suffixURL, NULL);
    }

#ifndef ARDUINO_AVR_UNO // Arduino Uno doesn't have enough memory to perform the following functionalities.

    /*
    Function: readMultipleFields

    Summary:
    Read all the field values, status message, location coordinates, and created-at timestamp associated with the latest feed to a private ThingSpeak channel and store the values locally in variables within a struct.

    Parameters:
    channelNumber - Channel number
    readAPIKey - Read API key associated with the channel. *If you share code with others, do _not_ share this key*

    Returns:
    HTTP status code of 200 if successful.

    Notes:
    See getLastReadStatus() for other possible return values.
    */
    void readMultipleFields(unsigned long channelNumber, const char *readAPIKey)
    {
        String readCondition = "/feeds/last.txt?status=true&location=true";
        stackofreturns.emplace([this]()
                               { readMultipleFields_1(); });
        readRaw(channelNumber, readCondition, readAPIKey);
    }
    void readMultipleFields_1()
    {
        if (getLastReadStatus() != TS_OK_SUCCESS)
        {
            stackofreturns.pop();
            onReadMultipleFieldsCallback(lastReadStatus);
            return;
        }

        this->lastFeed.nextReadField[0] = parseValues(httpresponsetext, "field1");
        this->lastFeed.nextReadField[1] = parseValues(httpresponsetext, "field2");
        this->lastFeed.nextReadField[2] = parseValues(httpresponsetext, "field3");
        this->lastFeed.nextReadField[3] = parseValues(httpresponsetext, "field4");
        this->lastFeed.nextReadField[4] = parseValues(httpresponsetext, "field5");
        this->lastFeed.nextReadField[5] = parseValues(httpresponsetext, "field6");
        this->lastFeed.nextReadField[6] = parseValues(httpresponsetext, "field7");
        this->lastFeed.nextReadField[7] = parseValues(httpresponsetext, "field8");
        this->lastFeed.nextReadCreatedAt = parseValues(httpresponsetext, "created_at");
        this->lastFeed.nextReadLatitude = parseValues(httpresponsetext, "latitude");
        this->lastFeed.nextReadLongitude = parseValues(httpresponsetext, "longitude");
        this->lastFeed.nextReadElevation = parseValues(httpresponsetext, "elevation");
        this->lastFeed.nextReadStatus = parseValues(httpresponsetext, "status");

        stackofreturns.pop();
        lastReadStatus = TS_OK_SUCCESS;
        onReadMultipleFieldsCallback(lastReadStatus);
    }

    /*
    Function: readMultipleFields

    Summary:
    Read all the field values, status message, location coordinates, and created-at timestamp associated with the latest update to a private ThingSpeak channel and store the values locally in variables within a struct.

    Parameters:
    channelNumber - Channel number
    readAPIKey - Read API key associated with the channel. *If you share code with others, do _not_ share this key*

    Returns:
    HTTP status code of 200 if successful.

    Notes:
    See getLastReadStatus() for other possible return values.
    */
    void readMultipleFields(unsigned long channelNumber)
    {
        readMultipleFields(channelNumber, NULL);
    }

    /*
    Function: getFieldAsString

    Summary:
    Fetch the value as string from the latest stored feed record.

    Parameters:
    field - Field number (1-8) within the channel to read from.

    Returns:
    Value read (UTF8 string), empty string if there is an error, or old value read (UTF8 string) if invoked before readMultipleFields().  Use getLastReadStatus() to get more specific information.
    */
    String getFieldAsString(unsigned int field)
    {
        if (field < FIELDNUM_MIN || field > FIELDNUM_MAX)
        {
            this->lastReadStatus = TS_ERR_INVALID_FIELD_NUM;
            return ("");
        }

        this->lastReadStatus = TS_OK_SUCCESS;
        return this->lastFeed.nextReadField[field - 1];
    }

    /*
    Function: getFieldAsFloat

    Summary:
    Fetch the value as float from the latest stored feed record.

    Parameters:
    field - Field number (1-8) within the channel to read from.

    Returns:
    Value read, 0 if the field is text or there is an error, or old value read if invoked before readMultipleFields(). Use getLastReadStatus() to get more specific information.  Note that NAN, INFINITY, and -INFINITY are valid results.
    */
    float getFieldAsFloat(unsigned int field)
    {
        return convertStringToFloat(getFieldAsString(field));
    }

    /*
    Function: getFieldAsLong

    Summary:
    Fetch the value as long from the latest stored feed record.

    Parameters:
    field - Field number (1-8) within the channel to read from.

    Returns:
    Value read, 0 if the field is text or there is an error, or old value read if invoked before readMultipleFields(). Use getLastReadStatus() to get more specific information.
    */
    long getFieldAsLong(unsigned int field)
    {
        // Note that although the function is called "toInt" it really returns a long.
        return getFieldAsString(field).toInt();
    }

    /*
    Function: getFieldAsInt

    Summary:
    Fetch the value as int from the latest stored feed record.

    Parameters:
    field - Field number (1-8) within the channel to read from.

    Returns:
    Value read, 0 if the field is text or there is an error, or old value read if invoked before readMultipleFields(). Use getLastReadStatus() to get more specific information.
    */
    int getFieldAsInt(unsigned int field)
    {
        // int and long are same
        return getFieldAsLong(field);
    }

    /*
    Function: getStatus

    Summary:
    Fetch the status message associated with the latest stored feed record.

    Results:
    Value read (UTF8 string). An empty string is returned if there was no status written to the channel or in case of an error.  Use getLastReadStatus() to get more specific information.
    */
    String getStatus()
    {
        return this->lastFeed.nextReadStatus;
    }

    /*
    Function: getLatitude

    Summary:
    Fetch the latitude associated with the latest stored feed record.

    Results:
    Value read (UTF8 string). An empty string is returned if there was no latitude written to the channel or in case of an error.  Use getLastReadStatus() to get more specific information.
    */
    String getLatitude()
    {
        return this->lastFeed.nextReadLatitude;
    }

    /*
    Function: getLongitude

    Summary:
    Fetch the longitude associated with the latest stored feed record.

    Results:
    Value read (UTF8 string). An empty string is returned if there was no longitude written to the channel or in case of an error.  Use getLastReadStatus() to get more specific information.
    */
    String getLongitude()
    {
        return this->lastFeed.nextReadLongitude;
    }

    /*
    Function: getElevation

    Summary:
    Fetch the longitude associated with the latest stored feed record.

    Results:
    Value read (UTF8 string). An empty string is returned if there was no elevation written to the channel or in case of an error.  Use getLastReadStatus() to get more specific information.
    */
    String getElevation()
    {
        return this->lastFeed.nextReadElevation;
    }

    /*
    Function: getCreatedAt

    Summary:
    Fetch the created-at timestamp associated with the latest stored feed record.

    Results:
    Value read (UTF8 string). An empty string is returned if there was no created-at timestamp written to the channel or in case of an error.  Use getLastReadStatus() to get more specific information.
    */
    String getCreatedAt()
    {
        return this->lastFeed.nextReadCreatedAt;
    }

#endif

    /*
    Function: getLastReadStatus

    Summary:
    Get the status of the previous read.

    Returns:
    Generally, these are HTTP status codes.  Negative values indicate an error generated by the library.
    Possible response codes...
    200 - OK / Success
    404 - Incorrect API key (or invalid ThingSpeak server address)
    -101 - Value is out of range or string is too long (> 255 characters)
    -201 - Invalid field number specified
    -210 - setField() was not called before writeFields()
    -301 - Failed to connect to ThingSpeak
    -302 -  Unexpected failure during write to ThingSpeak
    -303 - Unable to parse response
    -304 - Timeout waiting for server to respond
    -401 - Point was not inserted (most probable cause is exceeding the rate limit)

    Notes:
    The read functions will return zero or empty if there is an error.  Use this function to retrieve the details.
    */
    int getLastReadStatus()
    {
        return this->lastReadStatus;
    }

    void run()
    {
        if(!stackofreturns.empty())
        {
            std::invoke(stackofreturns.top());
        }
    }

    void onReadCallback(String response)
    {
    }
    void onWriteCallback(String response)
    {
    }
    void onWriteFieldsCallback(int response)
    {
    
    }
    void onWriteFieldCallback(int response)
    {
    }
    void onWriteRawCallback(int response)
    {
       
    }
    void onReadMultipleFieldsCallback(int status)
    {
    }
    void onReadStringFieldCallback(String response)
    {
    }
    void onReadFloatFieldCallback(float result)
    {
    }

    void onReadLongFieldCallback(long result)
    {
    }
    void onReadIntFieldCallback(int result)
    {
    }
    void onReadStatusCallback(String status)
    {
    }

    void onReadCreatedAtCallback(String result)
    {
    }

private:
    int getWriteFieldsContentLength()
    {
        size_t iField;
        int contentLen = 0;

        for (iField = 0; iField < FIELDNUM_MAX; iField++)
        {
            if (this->nextWriteField[iField].length() > 0)
            {
                contentLen = contentLen + 8 + this->nextWriteField[iField].length(); // &fieldX=[value]

                // future-proof in case ThingSpeak allows 999 fields someday
                if (iField > 9)
                {
                    contentLen = contentLen + 1;
                }
                else if (iField > 99)
                {
                    contentLen = contentLen + 2;
                }
            }
        }

        if (!isnan(this->nextWriteLatitude))
        {
            contentLen = contentLen + 5 + String(this->nextWriteLatitude).length(); // &lat=[value]
        }

        if (!isnan(this->nextWriteLongitude))
        {
            contentLen = contentLen + 6 + String(this->nextWriteLongitude).length(); // &long=[value]
        }

        if (!isnan(this->nextWriteElevation))
        {
            contentLen = contentLen + 11 + String(this->nextWriteElevation).length(); // &elevation=[value]
        }

        if (this->nextWriteStatus.length() > 0)
        {
            contentLen = contentLen + 8 + this->nextWriteStatus.length(); // &status=[value]
        }

        if (this->nextWriteTwitter.length() > 0)
        {
            contentLen = contentLen + 9 + this->nextWriteTwitter.length(); // &twitter=[value]
        }

        if (this->nextWriteTweet.length() > 0)
        {
            contentLen = contentLen + 7 + this->nextWriteTweet.length(); // &tweet=[value]
        }

        if (this->nextWriteCreatedAt.length() > 0)
        {
            contentLen = contentLen + 12 + this->nextWriteCreatedAt.length(); // &created_at=[value]
        }

        if (contentLen == 0)
        {
            return 0;
        }

        contentLen = contentLen + 13; // add 14 for '&headers=false', subtract 1 for missing first '&'

        return contentLen;
    }

    void emptyStream()
    {
        while (this->client->available() > 0)
        {
            this->client->read();
        }
    }

    void finishWrite()
    {
        // make sure all of the HTTP request is pushed out of the buffer before looking for a response
        this->client->flush();
        httpresponsetext = "";
        stackofreturns.emplace([this]()
                               { this->finishWrite_1(); });
        stackofreturns.emplace([this]()
                               { this->getHTTPResponse(); });
        timestamp1 = millis();
        getHTTPResponse();
    }
    void finishWrite_1()
    {
        emptyStream();
        stackofreturns.pop();

        if (getLastReadStatus() != TS_OK_SUCCESS)
        {
            this->client->stop();
            return;
        }
        long entryID = httpresponsetext.toInt();

#ifdef PRINT_DEBUG_MESSAGES
        Serial.print("               Entry ID \"");
        Serial.print(entryIDText);
        Serial.print("\" (");
        Serial.print(entryID);
        Serial.println(")");
#endif

        this->client->stop();

#ifdef PRINT_DEBUG_MESSAGES
        Serial.println("disconnected.");
#endif
        if (entryID == 0)
        {
            // ThingSpeak did not accept the write
            lastReadStatus = TS_ERR_NOT_INSERTED;
        }
        // return status;
    }

    String getJSONValueByKey(String textToSearch, String key)
    {
        if (textToSearch.length() == 0)
        {
            return String("");
        }

        String searchPhrase = String("\"");
        searchPhrase.concat(key);
        searchPhrase.concat("\":\"");

        int fromPosition = textToSearch.indexOf(searchPhrase, 0);

        if (fromPosition == -1)
        {
            // return because there is no status or it's null
            return String("");
        }

        fromPosition = fromPosition + searchPhrase.length();

        int toPosition = textToSearch.indexOf("\"", fromPosition);

        if (toPosition == -1)
        {
            // return because there is no end quote
            return String("");
        }

        textToSearch.remove(toPosition);

        return textToSearch.substring(fromPosition);
    }

#ifndef ARDUINO_AVR_UNO
    String parseValues(String &multiContent, String key)
    {
        if (multiContent.length() == 0)
        {
            return String("");
        }

        String searchPhrase = String("\"");
        searchPhrase.concat(key);
        searchPhrase.concat("\":\"");

        int fromPosition = multiContent.indexOf(searchPhrase, 0);

        if (fromPosition == -1)
        {
            // return because there is no status or it's null
            return String("");
        }

        fromPosition = fromPosition + searchPhrase.length();

        int toPosition = multiContent.indexOf("\"", fromPosition);

        if (toPosition == -1)
        {
            // return because there is no end quote
            return String("");
        }

        return multiContent.substring(fromPosition, toPosition);
    }
#endif

    int abortWriteRaw()
    {
        while (this->client->available() > 0)
        {
            this->client->read();
        }
        this->client->stop();
        resetWriteFields();

        return TS_ERR_UNEXPECTED_FAIL;
    }

    String abortReadRaw()
    {
        while (this->client->available() > 0)
        {
            this->client->read();
        }
        this->client->stop();
#ifdef PRINT_DEBUG_MESSAGES
        Serial.println("ReadRaw abort - disconnected.");
#endif
        this->lastReadStatus = TS_ERR_UNEXPECTED_FAIL;
        return String("");
    }

    void setPort(unsigned int port)
    {
        this->port = port;
    }

    void setClient(Client *client)
    {
        this->client = client;
    }

    Client *client = NULL;
    unsigned int port = THINGSPEAK_PORT_NUMBER;

    String httpresponsetext;
    std::stack<std::function<void()>> stackofreturns;
    unsigned long timestamp1;
    int contentLength;

    String nextWriteField[8];
    float nextWriteLatitude;
    float nextWriteLongitude;
    float nextWriteElevation;
    int lastReadStatus;
    String nextWriteStatus;
    String nextWriteTwitter;
    String nextWriteTweet;
    String nextWriteCreatedAt;
#ifndef ARDUINO_AVR_UNO
    feed lastFeed;
#endif

    bool connectThingSpeak()
    {
        bool connectSuccess = false;

#ifdef PRINT_DEBUG_MESSAGES
        Serial.print("               Connect to default ThingSpeak: ");
        Serial.print(THINGSPEAK_URL);
        Serial.print(":");
        Serial.print(this->port);
        Serial.print("...");
#endif

        connectSuccess = client->connect(const_cast<char *>(THINGSPEAK_URL), this->port);

#ifdef PRINT_DEBUG_MESSAGES
        if (connectSuccess)
        {
            Serial.println("Success.");
        }
        else
        {
            Serial.println("Failed.");
        }
#endif

        return connectSuccess;
    }

    bool writeHTTPHeader(const char *APIKey)
    {

        if (!this->client->print("Host: api.thingspeak.com\r\n"))
            return false;
        if (!this->client->print("User-Agent: "))
            return false;
        if (!this->client->print(TS_USER_AGENT))
            return false;
        if (!this->client->print("\r\n"))
            return false;
        if (NULL != APIKey)
        {
            if (!this->client->print("X-THINGSPEAKAPIKEY: "))
                return false;
            if (!this->client->print(APIKey))
                return false;
            if (!this->client->print("\r\n"))
                return false;
        }

        return true;
    }

    void getHTTPResponse()
    {
        if (this->client->available() < 17)
        {
            if (millis() - timestamp1 > TIMEOUT_MS_SERVERRESPONSE)
            {
                lastReadStatus = TS_ERR_TIMEOUT;
                stackofreturns.pop();
            }
            return;
        }

        if (!this->client->find(const_cast<char *>("HTTP/1.1")))
        {
#ifdef PRINT_HTTP
            Serial.println("ERROR: Didn't find HTTP/1.1");
#endif
            lastReadStatus = TS_ERR_BAD_RESPONSE;
            stackofreturns.pop();
            return;
        }

        lastReadStatus = this->client->parseInt();
#ifdef PRINT_HTTP
        Serial.print("Got Status of ");
        Serial.println(lastReadStatus);
#endif
        if (lastReadStatus != TS_OK_SUCCESS)
        {
            stackofreturns.pop();
            return;
        }

        // Find Content-Length
        if (!this->client->find(const_cast<char *>("Content-Length:")))
        {
#ifdef PRINT_HTTP
            Serial.println("ERROR: Didn't find Content-Length header");
#endif
            lastReadStatus = TS_ERR_BAD_RESPONSE;
            stackofreturns.pop();
            return; // Couldn't parse response (didn't find HTTP/1.1)
        }
        contentLength = this->client->parseInt();

#ifdef PRINT_HTTP
        Serial.print("Content Length: ");
        Serial.println(contentLength);
#endif

        if (!this->client->find(const_cast<char *>("\r\n\r\n")))
        {
#ifdef PRINT_HTTP
            Serial.println("ERROR: Didn't find end of headers");
#endif
            lastReadStatus = TS_ERR_BAD_RESPONSE;
            stackofreturns.pop();
            return;
        }
#ifdef PRINT_HTTP
        Serial.println("Found end of header");
#endif

        timestamp1 = millis();
        stackofreturns.pop();
        stackofreturns.emplace([this]()
                               { this->getHTTPResponse_1(); });
        getHTTPResponse_1();
    }
    void getHTTPResponse_1()
    {
        if (this->client->available() < contentLength)
        {
            if (millis() - timestamp1 > TIMEOUT_MS_SERVERRESPONSE)
            {
                lastReadStatus = TS_ERR_TIMEOUT;
                stackofreturns.pop();
                return;
            }
            return;
        }
        String tempString = String("");
        char y = 0;
        for (int i = 0; i < contentLength; i++)
        {
            y = client->read();
            tempString.concat(y);
        }
        httpresponsetext = tempString;

#ifdef PRINT_HTTP
        Serial.print("Response: \"");
        Serial.print(httpresponsetext);
        Serial.println("\"");
#endif
        stackofreturns.pop();
        return;
    }

    int convertFloatToChar(float value, char *valueString)
    {
        // Supported range is -999999000000 to 999999000000
        if (0 == isinf(value) && (value > 999999000000 || value < -999999000000))
        {
            // Out of range
            return TS_ERR_OUT_OF_RANGE;
        }
        // assume that 5 places right of decimal should be sufficient for most applications

#if defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_SAM)
        sprintf(valueString, "%.5f", value);
#else
        dtostrf(value, 1, 5, valueString);
#endif

        return TS_OK_SUCCESS;
    }

    float convertStringToFloat(String value)
    {
        // There's a bug in the AVR function strtod that it doesn't decode -INF correctly (it maps it to INF)
        float result = value.toFloat();

        if (1 == isinf(result) && *value.c_str() == '-')
        {
            result = (float)-INFINITY;
        }

        return result;
    }

    void resetWriteFields()
    {
        for (size_t iField = 0; iField < FIELDNUM_MAX; iField++)
        {
            this->nextWriteField[iField] = "";
        }
        this->nextWriteLatitude = NAN;
        this->nextWriteLongitude = NAN;
        this->nextWriteElevation = NAN;
        this->nextWriteStatus = "";
        this->nextWriteTwitter = "";
        this->nextWriteTweet = "";
        this->nextWriteCreatedAt = "";
    }
};

extern NonBlockingThingSpeakClass ThingSpeak;

#endif // ThingSpeak_h

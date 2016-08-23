/**
 * @author Guillermo A Torijano
 *
 * @section LICENSE
 *
 * Copyright (C) 2016 mbed
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * Lobby Matchmaking Server
 */

#ifndef QCLOUD_h

#include <string>
#include <vector>
#include <ctype.h>
#include <algorithm>
#include "mbed.h"
#include "EthernetInterface.h"
#include "HTTPClient.h"
#include "MbedJSONValue.h"

class QCLOUD
{   public:
        UDPSocket peer;
        pair <string,int> ipp;
        pair <string,int> ipp_ex;
        int punch;
        vector <string> hip;
        int online;
        int show_host;
        int host;
        string userother;
        string username;
        int keep_host;
        int end_host;
        string sid;
        int ble(string bub);
        QCLOUD():
            online(0),
            show_host(0),
            host(0),
            site("http://www.gatquest.com/server_work/storage.php?game="),
            timeout(10000){};
    ////
    private:
        EthernetInterface eth;
        HTTPClient http;
        int retry_cnt;
        int get_lk;
        int get_ip;
        int get_cs;
        int get_lh;
        int get_hs;
        int get_id;
        char result[128];
        string site;
        string request;
        int status;
        int timeout;
        string ip;
        int port;
        int lobby;
        string check;

        string string_replace_all(string str, char schr, char nchr);
        string string_digits(string str);
        vector <string> json_decode(string json);
        string data_to_string(pair <string,int> ipp, string name);
};

#endif

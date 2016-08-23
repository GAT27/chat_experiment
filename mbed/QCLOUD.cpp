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

#include "QCLOUD.h"

//Replace characters in a string with another
string QCLOUD::string_replace_all(string str, char schr, char nchr)
{   for (int i=0;str[i]!='\0';i++)
        if (str[i] == schr)
            str[i] = nchr;
    return str;
}
//Returns a string with only numbers
string QCLOUD::string_digits(string str)
{   string nstr;
    for (int i=0;str[i]!='\0';i++)
        if (isdigit(str[i]))
            nstr += str[i];
    return nstr;
}
//JSON array decoder (a:a:...:a)
vector <string> QCLOUD::json_decode(string json)
{   MbedJSONValue v;
    size_t t = count(json.begin(),json.end(),'\"')/4;
    vector <string> jparsed(t);
    size_t br = json.find_first_of("][");
    while (br != string::npos)
    {   json[br] = ' ';
        br = json.find_first_of("][",br+1);
    }
    json += "]}";
    json.insert(0,"{\"\":[");
    parse(v,json.c_str());
    for (int i=t-1;i>=0;i--)
        jparsed[i] = v[""][i*2].get<string>() + ':' + v[""][i*2+1].get<string>();
    return jparsed;
}
//Convert inputs into a string
string QCLOUD::data_to_string(pair <string,int> ipp, string name)
{   char p[8];
    sprintf(p,"%d",ipp.second);
    return "{ { "+ipp.first+','+p+','+name+" },  }";
}

/*Designed to work with ShadeSpeed CLOUD INDIE
http://gmc.yoyogames.com/index.php?showtopic=623357
PHP file moved to my "site"
This is for matchmaking and initial connection, does not provide netcode

Use 0 in HTTP event only
Returns 1 for completed connection
Returns 0 for pending connection
Returns -1 for error in connection
Use 1 to start
Use 2 to end
Use 99 in Alarm 1 to resend signals
Use id to select host

peer:       1st lv output, socket used to connect to others
ipp[0]:     1st lv output, ip address to connect to
ipp[1]:     1st lv output, port to connect into
ipp_ex[0]:  1st lv output, extra stored ip addresses
ipp_ex[1]:  1st lv output, extra stored ports
punch:      2nd lv output, special hole punch check plus input (see below)
hip[|]:     2nd lv output, list of clients on server: "user~lobby""{+{+ip,port,trade+},++}"
online:     3rd lv output, successful host/client connection
show_host:  3rd lv output, allow displaying of clients
host:       3rd lv output, if one connecting is hosting
userother:  3rd lv output, name of other connecting one
username:   input, name of self
.ble:       input, data to input from hip[|]
keep_host:  input, if hosting should be kept after a connection
end_host:   input, end communications during hosting and disable self join, ports are left opened
*/int QCLOUD::ble(string bub)
{   //Setups
    if (bub != "0")
    {   if (bub == "1")//Set up initial communication checks and begin
        {   /*punch = (p) changes over time to represent different changes
            p >= +3: (Can have additional functions)
            p == +2: Second stage check when joining host
                     (Can have additional functions)
            p == +1: First stage check when joining host or hosting
                     (Can have additional functions)
            p ==  0: Start requests or end online communications
            p == -1: Start online communications
                     (Can have additional functions)
            p <= -2: (Can have additional functions)
            */punch = 0;

            int peer_test = -1;
            host = 0;
            keep_host = 0;
            end_host = 0;
            retry_cnt = 5;
            get_lk = 0;
            get_ip = 1;
            get_cs = -1;
            get_lh = -1;
            get_hs = -1;
            get_id = 0;

            for (port=6510;peer_test<0;port++)
                peer_test = peer.bind(port);
            port--;
            printf("UDP: %d Name: %s\r\n",port,username.c_str());
            //lcd.cls();
            //lcd.printf("1ST REQUEST");
            request = site+sid+"&query=timeout_set_variable&variable="+string_replace_all(username,' ','+')
                    + "&value=user_ip&timeout=20&data=0";
            status = http.get(request.c_str(),result,timeout);
        }
        ////
        else if (bub == "99")//Retry request
        {   timeout += 1500;
            status = http.get(request.c_str(),result,timeout);
            if (get_id == get_ip)
                get_ip++;
            else if (get_id == get_cs)
                get_cs++;
            else if (get_id == get_lh)
                get_lh++;
            else if (get_id == get_hs)
                get_hs++;
        }
        ////
        else//Either end online communications early or set up host/client connection
        {   if (bub == "2")
            {   if (end_host)
                    peer.close();
            }
            else
            {   lobby = strtol(bub.substr(bub.find('~')+1).c_str(),NULL,10);
                userother = bub.substr(0,bub.find('~'));
                request = site+sid+"&query=timeout_all_variable&variable=main_lobby";
                get_lk *= -1;
                if (lobby <= 0)//Create a host
                {   host = 1;
                    status = http.get(request.c_str(),result,timeout);
                    get_lh = get_cs+1;
                    get_id++;
                    get_lk++;
                    if (lobby != 0)
                    {   keep_host = 1;
                        lobby = 0;
                    }
                }
                else//Join as a client
                {   int i = hip.size();
                    for (;strtol(hip[i].substr(0,hip[i].find(':')).substr(hip[i].find('~')+1).c_str(),NULL,10)!=lobby;i--);
                    string check2 = hip[i].substr(hip[i].find(':')+1);
                    check = check2.substr(4,check2.find(',')-4);
                    status = http.get(request.c_str(),result,timeout);
                    punch++;
                }
            }
            if (hip.size())
                hip.clear();
            if ((get_lk==3 and !end_host) or (online and !keep_host))
                punch = 0;
            online = 0;
            show_host = 0;
        }
        return 0;
    }

    //Matchmaking//----------------------------------------------------------------------------------------------------//
    get_id++;
    timeout -= 500;
    if (get_id == get_ip)//Capture current public ip and port
    {   //lcd.cls();
        //lcd.printf("GET_IP\n%d",retry_cnt);
        if (get_lk == 0)
        {   printf("STATUS: %d\r\n",http.getHTTPResponseCode());
            get_ip++;
            if (status == 0)
            {   //Confirm ip on server, go collect it
                if (strtol(string_digits(result).c_str(),NULL,10) == 1)
                {   request = site+sid+"&query=timeout_all_variable&variable="+string_replace_all(username,' ','+');
                    status = http.get(request.c_str(),result,timeout);
                }
                //Failure on ip confirm or collect, retry
                else if (strtol(string_digits(result).c_str(),NULL,10) == 0)
                {   retry_cnt--;
                    printf("I%d\r\n",retry_cnt);
                    if (retry_cnt)
                    {   wait(1.5);
                        QCLOUD::ble("99");
                    }
                }
                //Collected ip, store it for self and delete on server
                else
                {   retry_cnt = 5;
                    vector <string> tip = json_decode(result);
                    ip = tip[0].substr(0,tip[0].find(':'));
                    ipp.first = ip;
                    ipp.second = port;
                    printf("IP OBTAINED (1/2)\r\n");
                    printf("IP: %s\r\n",ipp.first.c_str());
                    printf("port: %d\r\n",ipp.second);
                    request = site+sid+"&query=timeout_delete_variable&variable="+string_replace_all(username,' ','+')
                            + "&value=user_ip";
                    status = http.get(request.c_str(),result,timeout);
                    get_cs = get_ip+1;
                    get_id++;
                    get_lk++;//LOCKED
                }
            }
            else//Error, retry
            {   retry_cnt--;
                printf("O%d\r\n",retry_cnt);
                if (retry_cnt)
                {   wait(1.5);
                    QCLOUD::ble("99");
                }
            }
        }
        else
        {   printf("OLD get_ip IGNORED\r\n");
            get_id++;
        }
    }
    //----------------------------------------------------------------------------------------------------//
    else if (get_id == get_cs)//Client selection on server to trade ip address with host
    {   //lcd.cls();
        //lcd.printf("GET_CS\n%d",retry_cnt);
        if (get_lk == 1)
        {   printf("STATUS: %d\r\n",http.getHTTPResponseCode());
            get_cs++;
            if (status == 0)
            {   //Confirm either deletion on server (collect lobbies) or ip trade (last check)
                if (strtol(string_digits(result).c_str(),NULL,10) == 1)
                {   if (punch <= 0)
                        printf("IP OBTAINED (2/2)\r\n");
                    else
                        printf("IP TRADED (1/2)\r\n");
                    request = site+sid+"&query=timeout_all_variable&variable=main_lobby";
                    status = http.get(request.c_str(),result,timeout);
                }
                //Failure on deletion, lobby collect, or trade confirm, retry
                else if ((strtol(string_digits(result).c_str(),NULL,10)==0) and (strcmp(result,"[]")!=0))
                {   retry_cnt--;
                    printf("I%d\r\n",retry_cnt);
                    if (retry_cnt)
                    {   wait(1.5);
                        QCLOUD::ble("99");
                    }
                }
                //Collected lobby, send to display, choose host, or finalize punch
                else
                {   retry_cnt = 5;
                    vector <string> tip = json_decode(result);
                    if (punch > 0)
                    {   int i = tip.size()-1;
                        for (;(i>=0) and (strtol(tip[i].substr(0,tip[i].find(':'))//Check if host is in lobby
                              .substr(tip[i].find('~')+1).c_str(),NULL,10)!=lobby);i--);
                        if (i >= 0)
                        {   string check2 = tip[i].substr(tip[i].find(':')+1);
                            if (punch == 1)//Check for no trade (same acquired ip)
                            {   if (check == check2.substr(4,check2.find(',')-4))
                                {   char l[4];
                                    sprintf(l,"%d",lobby);
                                    request = site+sid+"&query=timeout_set_variable&variable=main_lobby"
                                            + "&value="+string_replace_all(userother,' ','+')+'~'+l
                                            + "&timeout=20&data="+string_replace_all(data_to_string(ipp,username),' ','+');
                                    status = http.get(request.c_str(),result,timeout);
                                    check2 = check2.substr(check2.find(',')+1);
                                    ipp.first = check;
                                    ipp.second = strtol(check2.substr(0,check2.find(',')).c_str(),NULL,10);
                                    ipp_ex.first = ipp.first;
                                    ipp_ex.second = ipp.second;
                                    punch++;
                                    retry_cnt = 1;
                                }
                                else
                                {   printf("SOMEONE ELSE HAS ALREADY JOINED\r\n");
                                    retry_cnt = 0;
                                }
                            }
                            else if (punch == 2)//Check for trade (same owned ip)
                            {   if (ip == check2.substr(4,check2.find(',')-4))
                                {   punch = -1;
                                    online = 1;
                                    get_lk = 99;//LOCKED AND DONE
                                    printf("IP TRADED (2/2)\r\n");
                                }
                                else
                                {   printf("SOMEONE ELSE HAS JOINED\r\n");
                                    retry_cnt = 0;
                                }
                            }
                            else//Punch error
                            {   printf("PUNCH ERROR - JOINING HOST\r\n");
                                retry_cnt = 0;
                            }
                        }
                        else//Host gone
                        {   if (punch == 1)
                                printf("HOST IS ALREADY GONE\r\n");
                            else if (punch == 2)
                                printf("HOST IS GONE\r\n");
                            else
                                printf("PUNCH ERROR - FINDING HOST\r\n");
                            retry_cnt = 0;
                        }
                    }

                    else//Send to display
                    {   for (int i=0;i<tip.size();i++)
                        {   string check2 = tip[i].substr(tip[i].find(':')+1);
                            check2 = check2.substr(check2.find(',')+1);
                            check2 = check2.substr(check2.find(',')+1);
                            if (string_digits(check2) == "0")
                                hip.push_back(tip[i]);
                        }
                        show_host = 2;
                        get_lk *= -1;//TEMP LOCK
                        printf("SHOW LOBBY\r\n");
                    }
                }
            }
            else//Error, retry
            {   retry_cnt--;
                printf("O%d\r\n",retry_cnt);
                if (retry_cnt)
                {   wait(1.5);
                    QCLOUD::ble("99");
                }
            }
        }
        else
        {   printf("OLD get_cs IGNORED\r\n");
            get_id++;
        }
    }
    //----------------------------------------------------------------------------------------------------//
    else if (get_id == get_lh)//Check numbers of lobby and add host
    {   //lcd.cls();
        //lcd.printf("GET_LH\n%d",retry_cnt);
        if (get_lk == 2)
        {   printf("STATUS: %d\r\n",http.getHTTPResponseCode());
            get_lh++;
            if (status == 0)
            {   //Failure on lobby collect, retry
                if ((strtol(string_digits(result).c_str(),NULL,10)==0) and (strcmp(result,"[]")!=0))
                {   retry_cnt--;
                    printf("I%d\r\n",retry_cnt);
                    if (retry_cnt)
                    {   wait(1.5);
                        QCLOUD::ble("99");
                    }
                }
                //Collected lobby, find top lobby and create host on top
                else
                {   retry_cnt = 5;
                    vector <string> tip = json_decode(result);
                    if (tip.size())
                    {   string line = tip[tip.size()-1].substr(0,tip[tip.size()-1].find(':'));
                        lobby = strtol(line.substr(line.find('~')+1).c_str(),NULL,10) + 1;
                    }
                    else
                        lobby = 1;
                    char l[4];
                    sprintf(l,"%d",lobby);
                    printf("HOST ONLINE (1/2)\r\n");
                    printf("lobby: %s\r\n",l);
                    request = site+sid+"&query=timeout_set_variable&variable=main_lobby"
                            + "&value="+string_replace_all(username,' ','+')+'~'+l
                            + "&timeout=120&data="+string_replace_all(data_to_string(ipp,"0"),' ','+');
                    status = http.get(request.c_str(),result,timeout);
                    get_hs = get_lh+1;
                    get_id++;
                    get_lk++;//LOCKED
                }
            }
            else//Error, retry
            {   retry_cnt--;
                printf("O%d\r\n",retry_cnt);
                if (retry_cnt)
                {   wait(1.5);
                    QCLOUD::ble("99");
                }
            }
        }
        else
        {   printf("OLD get_lh IGNORED\r\n");
            get_id++;
        }
    }
    //----------------------------------------------------------------------------------------------------//
    else if (get_id == get_hs)//Keep checking host on server until client is captured
    {   //lcd.cls();
        //lcd.printf("GET_HS\n%d",retry_cnt);
        if (get_lk == 3)
        {   printf("STATUS: %d\r\n",http.getHTTPResponseCode());
            get_hs++;
            if (status == 0)
            {   //Confirm either host on server or client capture or host ending
                if (strtol(string_digits(result).c_str(),NULL,10) == 1)
                {   if (punch <= 0)//Hosting, go collect lobby
                    {   printf("HOST ONLINE (2/2)\r\n");
                        request = site+sid+"&query=timeout_all_variable&variable=main_lobby";
                        status = http.get(request.c_str(),result,timeout);
                        punch++;
                    }
                    else
                    {   if (!end_host)//Capturing, start hole punching
                        {   online = 1;
                            if (keep_host)
                            {   printf("CLIENT CAPTURED AND HOST UP\r\n");
                                request = site+sid+"&query=timeout_all_variable&variable=main_lobby";
                                status = http.get(request.c_str(),result,timeout);
                                online++;
                            }
                            else
                            {   punch = -1;
                                printf("CLIENT CAPTURED\r\n");
                            }
                        }
                        else//Ending, stop hosting
                        {   punch = 0;
                            printf("HOST ENDING\r\n");
                        }
                        if (!keep_host)
                            get_lk = 99;//LOCKED AND DONE
                    }
                }
                //Failure on hosting or lobby collect/delete, retry
                else if ((strtol(string_digits(result).c_str(),NULL,10)==0) and (strcmp(result,"[]")!=0))
                {   if ((punch<=0) or keep_host or end_host)
                    {   retry_cnt--;
                        printf("I%d\r\n",retry_cnt);
                    }
                    if (retry_cnt)
                    {   wait(1.5);
                        QCLOUD::ble("99");
                    }
                }
                //End hosting early
                else if (end_host)
                {   char l[4];
                    sprintf(l,"%d",lobby);
                    request = site+sid+"&query=timeout_delete_variable&variable=main_lobby"
                            + "&value="+string_replace_all(username,' ','+')+'~'+l;
                    status = http.get(request.c_str(),result,timeout);
                }
                //Collected lobby, find owned and see if there is a trade
                else
                {   retry_cnt = 5;
                    vector <string> tip = json_decode(result);
                    int i = tip.size()-1;
                    for (;(i>=0) and (strtol(tip[i].substr(0,tip[i].find(':'))
                           .substr(tip[i].find('~')+1).c_str(),NULL,10)!=lobby);i--);
                    if (i >= 0)
                    {   string client = tip[i].substr(tip[i].find(':')+1);
                        string check2 = client;
                        check2 = check2.substr(check2.find(',')+1);
                        check2 = check2.substr(check2.find(',')+1);
                        check2 = check2.substr(0,check2.find('}')-1);

                        if (check2 != "0")//If traded, capture client ip and port
                        {   userother = check2;
                            check2 = client.substr(client.find(',')+1);
                            ipp_ex.first = client.substr(4,client.find(',')-4);
                            ipp_ex.second = strtol(check2.substr(0,check2.find(',')).c_str(),NULL,10);
                            char l[4];
                            sprintf(l,"%d",lobby);

                            if (keep_host)//Keep host up after trade
                                request = site+sid+"&query=timeout_set_variable&variable=main_lobby"
                                        + "&value="+string_replace_all(username,' ','+')+'~'+l
                                        + "&timeout=120&data="+string_replace_all(data_to_string(ipp,"0"),' ','+');
                            else//Delete host after trade
                            {   ipp.first = ipp_ex.first;
                                ipp.second = ipp_ex.second;
                                request = site+sid+"&query=timeout_delete_variable&variable=main_lobby"
                                        + "&value="+string_replace_all(username,' ','+')+'~'+l;
                            }
                            status = http.get(request.c_str(),result,timeout);
                        }
                        else//Otherwise, retry later
                        {   wait(4.5);
                            QCLOUD::ble("99");
                        }
                    }
                    else//Host timed out
                    {   printf("HOST EXPIRED\r\n");
                        retry_cnt = 0;
                    }
                }
            }
            else//Error, retry
            {   if ((punch<=0) or keep_host or end_host)
                {   retry_cnt--;
                    printf("O%d\r\n",retry_cnt);
                }
                if (retry_cnt)
                {   wait(1.5);
                    QCLOUD::ble("99");
                }
            }
        }
        else
        {   printf("OLD get_hs IGNORED\r\n");
            get_id++;
        }
    }
    ////
    else
        printf("PAST REQUEST IGNORED\r\n");

    //Connection success, go online//----------------------------------------------------------------------------------------------------//
    if (online)
    {   if (get_lk == 99)//CONNECT LOCKDOWN
        {   get_lk++;
            return 1;
        }
        else if (online == 2)//CONFIRM PLUS CLIENT
        {   online--;
            return 1;
        }
        else//PENDING OTHERS
            return 0;
    }
    //Connection failure, return to offline
    else if ((get_lk>=0) and !retry_cnt)
    {   peer.close();
        punch = 0;
        printf("CONNECTION FAILED\r\n");
        get_lk = -1;//ERROR LOCKDOWN
        return -1;
    }
    //Connection still pending
    else
        return 0;
}

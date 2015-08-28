
/*
Based on ShadeSpeed CLOUD INDIE
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
hip[|]:     2nd lv output, list of clients on server
online:     3rd lv output, successful host/client connection
show_host:  3rd lv output, allow displaying of clients
host:       3rd lv output, if one connecting is hosting
userother:  3rd lv output, name of other connecting one
username:   input, name of self
.ble:       input, data to input from hip[|]
keep_host:  input, if hosting should be kept after a connection
end_host:   input, end communications during hosting and disable self join, ports are left opened
*/

//Setups
var bub = argument0;
if bub
{   switch bub
    {   //Set up initial communication checks and begin
        case 1:     /*punch = (p) changes over time to represent different changes
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
                    
                    peer = -1;
                    host = 0;
                    keep_host = 0;
                    end_host = 0;
                    retry_cnt = 5;
                    get_lk = 0;
                    get_ip = -1;
                    get_cs = -1;
                    get_lh = -1;
                    get_hs = -1;
                    site = "http://gatq.me/server_work/storage.php?game="+string(game_id);
                    
                    randomize();
                    for (port=6510+irandom(100);peer<0;port++)
                        peer = network_create_socket_ext(network_socket_udp,port);
                    port--;
                    show_debug_message("UDP " + string(port));
                    request = site+"&query=timeout_set_variable&variable="+username
                            + "&value=user_ip&timeout=20&data=0";
                    get_ip = http_get(request);
                    break;
                    
        //Retry request
        case 99:    switch get_id
                    {   case get_ip:    get_ip = http_get(request);
                                        break;
                        case get_cs:    get_cs = http_get(request);
                                        break;
                        case get_lh:    get_lh = http_get(request);
                                        break;
                        case get_hs:    get_hs = http_get(request);
                                        break;
                    }
                    break;
                    
        //Either end online communications early or set up host/client connection
        default:    if bub == 2
                    {   if end_host while peer >= 0
                        {   network_destroy(peer);
                            peer--;
                        }
                    }
                    else
                    {   lobby = real(string_delete(bub.ble,1,string_pos('~',bub.ble)));
                        userother = string_copy(bub.ble,1,string_pos('~',bub.ble)-1);
                        request = site+"&query=timeout_all_variable&variable=main_lobby";
                        get_lk *= -1;
                        
                        if !lobby//Create a host
                        {   host = 1;
                            get_lk++;
                            get_lh = http_get(request);
                        }
                        else//Join as a client
                        {   for (var i=ds_list_size(hip)-1;real(string_delete(ds_list_find_value(hip[|i],0),1,
                                                                              string_pos('~',ds_list_find_value(hip[|i],0))))
                                                                              !=lobby;i--){};
                            check = ds_list_find_value(hip[|i],1);
                            check2 = string_copy(check,5,string_pos(',',check)-5);
                            get_cs = http_get(request);
                            punch++;
                        }
                    }
                    
                    if hip
                    {   for (i=0;i<ds_list_size(hip);i++)
                            ds_list_destroy(hip[|i]);
                        ds_list_destroy(hip);
                        hip = -1;
                    }
                    if (get_lk==3 and !end_host) or online
                        punch = 0;
                    online = 0;
                    show_host = 0;
                    break;
    }
    
    exit;
}

//Matchmaking
get_id = async_load[?"id"];
switch get_id
{   //Capture current public ip and port
    case get_ip:    if get_lk == 0
                    {   status = async_load[?"http_status"];
                        show_debug_message("STATUS: " + string(status));
                        
                        if async_load[?"status"] == 0
                        {   ipp[0] = async_load[?"result"];
                            //Confirm ip on server, go collect it
                            if real(string_digits(ipp[0])) == 1
                            {   request = site+"&query=timeout_all_variable&variable="+username;
                                get_ip = http_get(request);
                            }
                            //Failure on ip confirm or collect, retry
                            else if real(string_digits(ipp[0])) == 0
                            {   retry_cnt--;
                                alarm[1] = room_speed * 2;
                                show_debug_message('I'+string(retry_cnt));
                            }
                            //Collected ip, store it for self and delete on server
                            else
                            {   retry_cnt = 5;
                                var tip = ds_list_create();
                                tip = ds_map_find_value(json_decode(ipp[0]),"default");
                                ip = ds_list_find_value(tip[|0],0);
                                ipp[0] = ip;
                                ipp[1] = port;
                                for (var i=0;i<ds_list_size(tip);i++)
                                    ds_list_destroy(tip[|i]);
                                ds_list_destroy(tip);
                                
                                request = site+"&query=timeout_delete_variable&variable="+username+"&value=user_ip";
                                get_cs = http_get(request);
                                show_debug_message("IP OBTAINED (1/2)");
                                get_lk++;//LOCKED
                            }
                        }
                        
                        else//Error, retry
                        {   retry_cnt--;
                            alarm[1] = room_speed * 2;
                            show_debug_message('O'+string(retry_cnt));
                        }
                    }
                    else
                        show_debug_message("OLD get_ip IGNORED");
                    break;
                    
    //Client selection on server to trade ip address with host
    case get_cs:    if get_lk == 1
                    {   status = async_load[?"http_status"];
                        show_debug_message("STATUS: " + string(status));
                        
                        if async_load[?"status"] == 0
                        {   ipp[2] = async_load[?"result"];
                            //Confirm either deletion on server (collect lobbies) or ip trade (last check)
                            if real(string_digits(ipp[2])) == 1
                            {   request = site+"&query=timeout_all_variable&variable=main_lobby";
                                get_cs = http_get(request);
                                if !punch
                                    show_debug_message("IP OBTAINED (2/2)");
                                else
                                    show_debug_message("IP TRADED (1/2)");
                            }
                            //Failure on deletion, lobby collect, or trade confirm, retry
                            else if real(string_digits(ipp[2]))==0 and string(ipp[2])!="[]"
                            {   retry_cnt--;
                                alarm[1] = room_speed * 2;
                                show_debug_message('I'+string(retry_cnt));
                            }
                            //Collected lobby, send to display, choose host, or finalize punch
                            else
                            {   retry_cnt = 5;
                                var tip = ds_list_create();
                                tip = ds_map_find_value(json_decode(ipp[2]),"default");
                                
                                if punch
                                {   var line = ds_list_find_value(tip[|ds_list_size(tip)-1],0);
                                    for (var i=ds_list_size(tip)-1;real(string_delete(line,1,string_pos('~',line)))!=lobby;i--)
                                    {   if i<0 break;//Check if host is in lobby
                                        else
                                            line = ds_list_find_value(tip[|i],0);
                                    }
                                    
                                    if i >= 0
                                    {   check = ds_list_find_value(tip[|i],1);
                                        if punch == 1//Check for no trade (same acquired ip)
                                        {   if check2 == string_copy(check,5,string_pos(',',check)-5)
                                            {   ipp[2] = username;
                                                request = site+"&query=timeout_set_variable&variable=main_lobby"
                                                        + "&value="+userother+'~'+string(lobby)+"&timeout=20&data="+string(ipp);
                                                get_cs = http_get(request);
                                                
                                                var check3 = string_delete(check,1,string_pos(',',check));
                                                ipp[0] = string_copy(check,5,string_pos(',',check)-5);
                                                ipp[1] = real(string_copy(check3,1,string_pos(',',check3)-1));
                                                punch++;
                                                retry_cnt = 0;
                                            }
                                            else
                                            {   show_debug_message("SOMEONE ELSE HAS ALREADY JOINED");
                                                retry_cnt = -1;
                                            }
                                        }
                                        else if punch == 2//Check for trade (same owned ip)
                                        {   if ip == string_copy(check,5,string_pos(',',check)-5)
                                            {   punch = -1;
                                                online = 1;
                                                get_lk = 99;//LOCKED AND DONE
                                                show_debug_message("IP TRADED (2/2)");
                                            }
                                            else
                                            {   show_debug_message("SOMEONE ELSE HAS JOINED");
                                                retry_cnt = -1;
                                            }
                                        }
                                        else//Punch error
                                        {   show_debug_message("PUNCH ERROR - JOINING HOST");
                                            retry_cnt = -1;
                                        }
                                    }
                                    
                                    else//Host gone
                                    {   if punch == 1
                                            show_debug_message("HOST IS ALREADY GONE");
                                        else if punch == 2
                                            show_debug_message("HOST IS GONE");
                                        else
                                            show_debug_message("PUNCH ERROR - FINDING HOST");
                                        retry_cnt = -1;
                                    }
                                }
                                ////
                                else//Send to display
                                {   hip = ds_list_create();
                                    for (var i=0;i<ds_list_size(tip);i++)
                                    {   check = ds_list_find_value(tip[|i],1);
                                        var check3 = check;
                                        repeat 2 check3 = string_delete(check3,1,string_pos(',',check3));
                                        if string_copy(check3,1,string_pos('}',check3)-2) == '0'
                                        {   var h = ds_list_create();
                                            ds_list_copy(h,tip[|i]);
                                            ds_list_add(hip,h);
                                        }
                                    }
                                    show_host = 2;
                                    get_lk *= -1;//TEMP LOCK
                                    show_debug_message("SHOW LOBBY");
                                }
                                
                                for (i=0;i<ds_list_size(tip);i++)
                                    ds_list_destroy(tip[|i]);
                                ds_list_destroy(tip);
                            }
                        }
                        
                        else//Error, retry
                        {   retry_cnt--;
                            alarm[1] = room_speed * 2;
                            show_debug_message('O'+string(retry_cnt));
                        }
                    }
                    else
                        show_debug_message("OLD get_cs IGNORED");
                    break;
                    
    //Check numbers of lobby and add host
    case get_lh:    if get_lk == 2
                    {   status = async_load[?"http_status"];
                        show_debug_message("STATUS: " + string(status));
                        
                        if async_load[?"status"] == 0
                        {   lobby = async_load[?"result"];
                            //Failure on lobby collect, retry
                            if real(string_digits(lobby))==0 and string(lobby)!="[]"
                            {   retry_cnt--;
                                alarm[1] = room_speed * 2;
                                show_debug_message('I'+string(retry_cnt));
                            }
                            //Collected lobby, find top lobby and create host on top
                            else
                            {   retry_cnt = 5;
                                var tip = ds_list_create();
                                tip = ds_map_find_value(json_decode(lobby),"default");
                                if ds_list_size(tip)
                                {   var line = ds_list_find_value(tip[|ds_list_size(tip)-1],0);
                                    lobby = real(string_delete(line,1,string_pos('~',line))) + 1;
                                }
                                else
                                    lobby = 1;
                                ipp[2] = 0;
                                for (var i=0;i<ds_list_size(tip);i++)
                                    ds_list_destroy(tip[|i]);
                                ds_list_destroy(tip);
                                
                                request = site+"&query=timeout_set_variable&variable=main_lobby"
                                        + "&value="+username+'~'+string(lobby)+"&timeout=120&data="+string(ipp);
                                get_hs = http_get(request);
                                show_debug_message("HOST ONLINE (1/2)");
                                get_lk++;//LOCKED
                            }
                        }
                        
                        else//Error, retry
                        {   retry_cnt--;
                            alarm[1] = room_speed * 2;
                            show_debug_message('O'+string(retry_cnt));
                        }
                    }
                    else
                        show_debug_message("OLD get_lh IGNORED");
                    break;
                    
    //Keep checking host on server until client is captured
    case get_hs:    if get_lk == 3
                    {   status = async_load[?"http_status"];
                        show_debug_message("STATUS: " + string(status));
                        
                        if async_load[?"status"] == 0
                        {   var client = async_load[?"result"];
                            //Confirm either host on server or client capture or host ending
                            if real(string_digits(client)) == 1
                            {   if !punch//Hosting, go collect lobby
                                {   request = site+"&query=timeout_all_variable&variable=main_lobby";
                                    get_hs = http_get(request);
                                    punch++;
                                    show_debug_message("HOST ONLINE (2/2)");
                                }
                                else
                                {   if !end_host//Capturing, start hole punching
                                    {   online = 1;
                                        if keep_host
                                        {   request = site+"&query=timeout_all_variable&variable=main_lobby";
                                            get_hs = http_get(request);
                                            online++;
                                            show_debug_message("CLIENT CAPTURED AND HOST UP");
                                        }
                                        else
                                        {   punch = -1;
                                            show_debug_message("CLIENT CAPTURED");
                                        }
                                    }
                                    else//Ending, stop hosting
                                    {   punch = 0;
                                        show_debug_message("HOST ENDING");
                                    }
                                    if !keep_host
                                        get_lk = 99;//LOCKED AND DONE
                                }
                            }
                            //Failure on hosting or lobby collect/delete, retry
                            else if real(string_digits(client))==0 and string(client)!="[]"
                            {   if !punch or keep_host or end_host
                                {   retry_cnt--;
                                    show_debug_message('I'+string(retry_cnt));
                                }
                                alarm[1] = room_speed * 2;
                            }
                            //End hosting early
                            else if end_host
                            {   request = site+"&query=timeout_delete_variable&variable=main_lobby"
                                        + "&value="+username+'~'+string(lobby);
                                get_hs = http_get(request);
                            }
                            //Collected lobby, find owned and see if there is a trade
                            else
                            {   retry_cnt = 5;
                                var tip = ds_list_create();
                                tip = ds_map_find_value(json_decode(client),"default");
                                for (var i=ds_list_size(tip)-1;real(string_delete(ds_list_find_value(tip[|i],0),1,
                                                                                  string_pos('~',ds_list_find_value(tip[|i],0))))
                                                                                  !=lobby;i--)
                                    if i<0 break;//Check if host is in lobby
                                
                                if i >= 0
                                {   client = ds_list_find_value(tip[|i],1);
                                    var check3 = client;
                                    repeat 2 check3 = string_delete(check3,1,string_pos(',',check3));
                                    ipp[2] = string_copy(check3,1,string_pos('}',check3)-2);
                                    for (i=0;i<ds_list_size(tip);i++)
                                        ds_list_destroy(tip[|i]);
                                    ds_list_destroy(tip);
                                    
                                    if ipp[2] != '0'//If traded, capture client ip and port
                                    {   check3 = string_delete(client,1,string_pos(',',client));
                                        userother = ipp[2];
                                        if keep_host//Keep host up after trade
                                        {   ipp_ex[0] = string_copy(client,5,string_pos(',',client)-5);
                                            ipp_ex[1] = real(string_copy(check3,1,string_pos(',',check3)-1));
                                            ipp[2] = 0;
                                            request = site+"&query=timeout_set_variable&variable=main_lobby"
                                                    + "&value="+username+'~'+string(lobby)+"&timeout=120&data="+string(ipp);
                                        }
                                        else//Delete host after trade
                                        {   ipp[0] = string_copy(client,5,string_pos(',',client)-5);
                                            ipp[1] = real(string_copy(check3,1,string_pos(',',check3)-1));
                                            request = site+"&query=timeout_delete_variable&variable=main_lobby"
                                                    + "&value="+username+'~'+string(lobby);
                                        }
                                        get_hs = http_get(request);
                                    }
                                    else//Otherwise, retry later
                                        alarm[1] = room_speed * 5;
                                }
                                
                                else//Host timed out
                                {   show_debug_message("HOST EXPIRED");
                                    retry_cnt = -1;
                                }
                            }
                        }
                        
                        else//Error, retry
                        {   if !punch or keep_host or end_host
                            {   retry_cnt--;
                                show_debug_message('O'+string(retry_cnt));
                            }
                            alarm[1] = room_speed * 2;
                        }
                    }
                    else
                        show_debug_message("OLD get_hs IGNORED");
                    break;
                    
    default:        show_debug_message("PAST REQUEST IGNORED");
}

//Connection success, go online
if online
{   if retry_cnt < 0
        alarm[1] = -1;
    
    if get_lk == 99//CONNECT LOCKDOWN
    {   get_lk++;
        return 1;
    }
    else if online == 2//CONFIRM PLUS CLIENT
    {   online--;
        return 1;
    }
    else//PENDING OTHERS
        return 0;
}
//Connection failure, return to offline
else if (get_lk>=0) and (retry_cnt<0)
{   network_destroy(peer);
    punch = 0;
    alarm[1] = -1;
    show_debug_message("CONNECTION FAILED");
    get_lk = -1;//ERROR LOCKDOWN
    return -1;
}
//Connection still pending
else
    return 0;
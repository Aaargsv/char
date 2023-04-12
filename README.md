This is text chat with sfml-audio connection

Specify server ip and port as command line arguments <br/>
example: server.exe 127.0.0.1 1024 <br/>
example: client.exe 127.0.0.1 1024 <br/> 
Or specify file with ip and port settings <br/>
example: client.exe ip_port.txt <br/>
if nothing is specified then the IP and port are taken from the file settings.txt that should be in the same directory as the executable file

To send a message to a user, enter his name at the beginning of message <br/>
Message formats:
1) For text message: USERNAME {YOUR MESSAGE} <br/> 
   
   example: Alexey hello! <br/>
   if you want broadcast, use USERNAME - all <br />
   example: all hello! <br/> <br/>
   
2) For file transfer: USERNAME [file] {PATH TO YOUR FILE} <br />
   example: Alexey [file] ..\file\config.txt <br/> <br/>

3) For audio communication: USERNAME sound on <br/>
   example: Alexey sound on <br/> <br/>

4) If you want to disconnect from audio: USERNAME sound off <br/>
   example: Alexey sound off
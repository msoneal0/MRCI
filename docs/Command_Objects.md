### 3.1 Command Objects ###

```ExternCommand``` objects are QT/C++ classes used by the host to execute all logic related to the specific commands called by the client. It basically defines several QT signals and virtual functions that the host's internal ```CmdExecutor``` object uses to process input data from the client and return data to the client or to other peers connected to the host. Each command callable by the client defines a seperate ```ExternCommand``` object and it's life cycle is managed externally by the host's ```CmdExecutor``` object.

### 3.2 Defined Variables in ExternCommand ###

Aside from the virtual functions and QT signals, the ```ExternCommand``` object also defines a few public variables that can be of interest:

```cmdId```
This is a ```quint16``` command id that the command object was assigned to when it was built. Changing it at any time does nothing upstream but it is reset to it's proper value every time the ```procBin()``` function is called. the MRCI protocol use assigned command ids to call certain command objects currently loaded into the ```CmdExecutor``` object. It is unique to each command object and makes it possible for the client to call a command with only 2 bytes of data instead of a variable number of bytes for a command name. This variable exist to make it possible for the internal logic of the command object to be made aware of it's assigned command id.

```internCommands```
This is a ```QHash<QString, ExternCommand*>``` containing a list of pointers to internal command objects built to be used from within this command object. It is filled by the ```internRequest()``` function (section 3.3). Internal commands differ from external commands like this by having direct access to the host database and less restrictive access to various host functions. This variable is useful if you want access to such functions without the chance of causing a session crash or database corruption.

```errSent```
This is a ```bool``` that simply indicates if the command has sent error message to the client using the ```errTxt()``` non-virtual function. This is useful if you want to determine of the last run of the command was successful or not but only if the internal logic used the aforementioned function to send the error. The ```CmdExecutor``` does reset it to false before calling the ```procBin()``` function.

```inLoopMode```
This is a ```bool``` that indicates if the command object is currently in loop mode. This basically retains what was passed into the ```enableLoop()``` signal when it is called. more info on this can be found in section 3.5.

```inMoreInputMode```
This is a ```bool``` that indicates if the command object is currently in the more input state. It retains what was passed into the ```enableMoreInput()``` signal when it is called. see section 3.5 for more info.

```QString QObject::objectName()```
This is not a variable but it is a function available to the ```ExternCommand``` object from the QT API. The ```CmdExecutor``` assigns the unique command name to this property that was assigned to this command when it was built. Just like ```cmdId``` this makes the internal logic aware of it's assigned command name. Changing it using ```QObject::setObjectName()``` does nothing upstream but also unlike ```cmdId``` it is not reset upon calling ```procBin()```.

### 3.3 ExternCommand Virtual Functions ###

```void procBin(SharedObjs,QByteArray,uchar)```
This function is called by the host to process input data from the client passed in the form of a ```QByteArray```. Look at this as the main execution function of the command that can be called more than once if the loop state is enabled. The ```SharedObjs``` parameter is passed by the ```CmdExecutor``` to share information about the session with the command object as described in section 3.4 of this document. It is passed as a const because changing any of the objects within ```SharedObjs``` in an unexpected way may cause undesired behaviour. The third ```uchar``` parameter indicate what type of data is in the ```QByteArray```. Details on type ids can be found in the [Type_IDs.md](Type_IDs.md) document.

```bool handlesGenfile()```
Command objects should return true on this function if your command object handles the ```GEN_FILE``` data type in anyway. The host will use this to tell the client which commands support this data type which ones don't. The reason for this is because the ```GEN_FILE``` data type require structured 2 way communication in a way between the host and client (see [Type_IDs.md](Type_IDs.md)). This function is called only once shortly after the command is contructed.

```QString shortText()```
Command objects must return a short summary on what this command actually does. The host will use this to send help text to the client. How this text is displayed to the user depends entirely on the client.

```QString ioText()```
Just like shortText(), this is a help text function that describes what input/output data to expected to/from the command object. It follows a specific text format:

```
  [] Brackets are used to indicate a block of data (text,binary,etc..). 
  Example: [text]
  
  / Seperates input and output data blocks to/from the command. 
  Example: [input_text]/[output_text]
  
  - Indicates a argument.
  Example: [-arg]/[text]
  
  () Brackets indicate argument parameters/values.
  Example: [-arg (text)]/[text]
  
  {} Brackets indicate optional arguments or parameters.
  Example: [-arg {(text)} {-optional}]/[text]
  
  'or' Indicates optional input data blocks and/or possible output blocks. 
  Example: [-arg1 (text)] or [-arg2 (text)]/[output_text1] or [output_text2]
```

```QString longText()```
This is a help text function that is used by the host to send full detailed information about the command and it's usage. It's recommended to be as thorough as possible to help users understand proper usage of your command. The host will send it as ```BIG_TEXT``` so it will be up to the client if it should word-wrap the text when displaying to the user.

```QString libText()```
Command objects can use this function to return the library/module name and version that the command object belongs to. This can also contain extra information that clients can use to further identify the command object; however, the text is limited to 64 chars or less. Any text returned by this function that is larger than 64 chars will be truncated.

```QStringList internRequest()```
After the command is successfully built, this function is called by the ```CmdExecutor``` to fulfil any request to build internal command objects that can be used from within the object that was built. the ```QStringList``` returned by this function just need to list any of the known internal command names and the ```CmdExecutor``` will build them and add them to the ```internCommands``` ```QHash``` defined inside the ```ExternCommand``` object (section 3.2). Here's a few notes to consider if you want to build internal commands within your own command.

* Your command object is assigned the parent of the internal commands.
* The internal commands built into internCommands have full access to the internal host batabase and functions that normal ExternCommands would otherwise not have access to.
* The output from the internal commands go directly to the client, peers or other objects. Nothing is passed through the parent object by default.
* Clients do not have direct access to these specific internal commands. Instead, they can only be called by running the ```procBin()``` function directly from within the parent object.
* The internal commands inherit the parent command object's command id/name so output from these internal commands will appear as if it came from the parent object. This also includes the ```enableLoop()``` and ```enableMoreInput()``` signals (section 3.5).
* It will be up to the parent object to detect and execute the loop mode for these internal commands since ```CmdExecutor::exeCmd()``` will not call these objects directly.
* Terminating the parent object also terminates all of the internal commands within it.
* It is safe to delete the internals commands but be sure to remove the deleted commands from the ```internCommands``` ```QHash``` or it will must likely cause the session to crash. You normally should never have to do this anyway, the ```CmdExecutor``` will safely delete these internal commands when the parent is about to get deleted.
    
```void term()```
This function is called by the host when the command needs to terminate what it is currently doing. Use this opportunity to reset and clean up any reasources so the command object can be ready to be called again in a reset state.

```void aboutToDelete()```
The host will call this function before calling the command object's ```deleteLater()``` function. Use this opportunity to clean up anything the host may not be aware of in preparation for deletion. Avoid self deleting any ```ExternCommand``` object at any time, doing so will must certainly cause the session to crash.

```bool errState();```
Return true or false on this function to indicate if the command object is currently in an error state. The default logic simply returns ```errSent``` (section 3.2).

### 3.4 Shared Objects ###

The ```SharedObjs``` class is used by the host's ```Session``` and ```CmdExecutor``` objects to share data with all command objects. It contains pointers to various objects related to the user's current session and command states. It is passed into the command via the ```ExternCommand::procBin()``` function as a read only const. Below are all of the pointers defined within it:

```sessionId```
This is a ```QByteArray``` containing the 224bit (28byte) sha3 hash id unique to the current session only. this does change upon reconnecting to the host.

```p2pAccepted```
This is a ```QList<QByteArray>``` object that list all peer session ids that the user has accepted to be allowed to send/receive data when using the ```toPeer()``` signal.

```p2pPending```
This is a ```QList<QByteArray>``` object that list all peer session ids that sent a ```P2P_REQUEST``` to/from the session and are awaiting a ```P2P_OPEN```. When a ```P2P_OPEN``` is received, the session id sent by it is removed from this list and then added to ```p2pAccepted```.

```chIds```
This is a ```QByteArray``` object that string togeather up to 6 channel-sub combinations that indicate which channel id and sub id combinations are currently open. Each channel-sub are 9bytes long and chIds itself maintians a fixed length of 54bytes. It is padded with 0x00 chars to maintain the fixed length (this padding can appear anywhere in 9byte increments within chIds). Channel-sub is formatted like this:

```
  bytes[0-7] - 64bit LE unsigned int (channel id)
  bytes[8]   - 8bit LE unsigned int (sub id)

  note: channel id 0 is not a valid id and the sub id cannot
        be valid without a valid channel id.
```

```wrAbleChIds```
This is a ```QByteArray``` object formatted exactly like ```chIds```. It however list all channel-subs from ```chIds``` that are 'writable.' This is what is actually used when casting data to peers while ```chIds``` is used when receiving data from peers. This cannot list any channel-subs that are not also listed in ```chIds```.

```chList```
This is a ```QList<QString>``` of channel names that the currently logged user is a member of. Unlike ```chIds```, this list doesn't care if the channel is open or not.

```activeUpdate```
This is a ```bool``` object that indicate if the above ```chIds``` contains an open active update channel. When a session is active updating it will respond to peer pings, ```PEER_INFO``` and ```PEER_STAT``` frames related to just the active update sub-channels.

```chOwnerOverride```
This is a ```bool``` object that indicates if the user currently have the channel owner override flag active or not. The owner override flag allows the user to do anything a channel owner could do in a channel without actually being the owner of the channel. This is usually reserved for host administrators.

```sessionAddr```
This is a ```QString``` representation of the IPv4 or IPv6 address of the TCP client.

```userName```
This is a ```QString``` of the current user that is logged in on the current session. This is empty if no user is currently logged in.

```groupName```
This is a ```QString``` of the group that the above ```userName``` is a member of. it is also blank if no user is currently logged in.

```displayName```
This is a ```QString``` of the user's desired display name. Clients are encouraged put more emphasis on this if displaying the user to the public.

```appName```
This is a ```QString``` of the client's application name. It can also contain the application's version depending on the application itself. This is updated upon connecting to the host and normally does not change while the session is active.

```clientMajor```
This is a ```ushort``` version major of the mrci protocol that the client is using.

```clientMinor```
This is a ```ushort``` version minor of the mrci protocol that the client is using.

```clientPatch```
This is a ```ushort``` version patch of the mrci protocol that the client is using.

```userId```
This is a ```QByteArray``` containing the 256bit (32byte) Keccak hash unique to the current user. This is empty if no user is currently logged in. This remains constant even when the user name, or any other user information changes.

```moreInputCmds```
This is a ```QList<quint16>``` of command ids that are currently in the more input state. Command objects in this state are simply requesting more input data from the client and should not be considered finished.

```activeLoopCmds```
This is a ```QList<quint16>``` of command ids that are currently in the loop state. Command objects in this state will cause the ```CmdExecutor``` to call ```ExternCommand::procBin()``` in a loop until it is removed from this list.

```pausedCmds```
This is a ```QList<quint16>``` containing a list of command ids that are blocked from looping. The commands listed here must also be listed in activeLoopCmds.

```hostRank```
This is a ```uint``` that indicates the currently logged in user's host rank. The host rank determines the level of access to the host commands the user currently have. what those commands could be depends on how the host is configured. The lower the rank value, the higher the privilege level with 1 being the highest level. 0 is not considered a valid rank and it will initialized as that if no user is currently logged in.

```cmdNames```
This is a ```QHash<quint16,QString>``` with command id ```quint16``` keys that relate to ```QString``` command names that are currently loaded for the session.

### 3.5 ExternCommand Signals ###

```dataToClient(QByteArray data, uchar typeId)```
This signal sends the data to ```CmdExecutor``` passed as a ```QByteArray``` with a ```uchar``` indicating what [TypeID](Type_IDs.md) is stored in that ```QByteArray```. The ```CmdExecutor``` object inspects the type id and stops external commands from sending the following data types: ```PRIV_IPC```, ```PUB_IPC```, ```NEW_CMD```, ```PEER_STAT```, ```MY_INFO```, ```PEER_INFO```, ```HOST_CERT```, ```IDLE```, ```PING_PEERS```, ```P2P_REQUEST```, ```P2P_OPEN```, ```P2P_CLOSE```. If none of these type ids are caught, the data is then sent to the client as a mrci frame.

```castToPeers(QByteArray data, uchar typeId)```
This is used to broadcast data to all clients that have any matching sub-channels open. It takes the same parameters and have the same retrictions as ```dataToClient()```.

```toPeer(QByteArray sessionId, QByteArray data, uchar typeId)```
This signal is used to send data directly to a peer client without the need to open any sub-channels. Instead, the data is routed via the target peer's session id, given in the 1st ```QByteArray``` parameter as 224bit (28byte) sha3 hash. The next 2 parameters are the same as ```dataToClient()```, however only the following data types are allowed until a connection is negotiated with the peer: ```P2P_REQUEST```, ```P2P_OPEN``` or ```P2P_CLOSE```. More info on this can be found in [Type_IDs.md](Type_IDs.md), section 4.2. Except the P2P specific data types, the same retrictions for ```dataToClient()``` also apply here.

```closeChById(quint64 channelId, uchar subId), closeChByName(QString channelName, QString subName), openChById(quint64 channelId, uchar subId), openChByName(QString channelName, QString subName)```
These signals open/close sub-channels via the channel and sub-channel names or ids. The ```CmdExecutor``` does error checking on these so errors are returned to the client if the channel or sub-channel does not exists, is invalid or if the user doesn't have access to it.

```enableLoop(bool state)```
This takes a ```bool``` value that enables or disables the command object's loop state. If the loop state is enabled, the ```procBin()``` function will be called by the ```CmdExecutor``` within it's own loop until the loop state is disabled. 3rd party module devs are highly recommended to use this rather than implementing their own long term loops. By letting the ```CmdExecutor``` handle the looping structure it would be possible for it terminate/interrupt the command more gracefully from the outside.

```enableMoreInput(bool state)```
This takes a ```bool``` value that enables or disables the command object's more input state. This state tells the ```CmdExecutor``` that the command is requesting more data from the client and should hold off sending an ```IDLE``` frame to the client because the command is not yet finished. if both loop and more input states are disabled, an ```IDLE``` frame is sent to indicate to the client that the command is finished.

```closeSession()```
This closes the session from the host side. The TCP connection also closes.

```cmdFinished()```
This signal disables both loop and more input states. It also immediately sends the IDLE frame to the client to indicate that the command is finished. Normally, you should not have to emit this signal directly because the ```CmdExecutor``` will automatically send the ```IDLE``` frame if it detects that both the loop and more inputs states are disabled after calling ```procBin()```. This signal is only useful if you need to indicate that the command is finished outside of the ```procBin()``` function.

```logout()```
This clears parameters in ```SharedObjs``` related to the currently logged in user and puts the session in a reset state with no user currently logged in. Unlike ```closeSession()```, this does not close the TCP connection.
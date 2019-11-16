### 1.1 The Protocol ###

The main goal of this application is to transport data from remote TCP clients to the [Modules](modules.md) defined in the host. How the data is processed and/or returned back to the client depends entirely on the type of data being transported or the module itself. The data format that the host understands for data transport is referred to as MRCI frames described below in section 1.2.

Before any MRCI frames can be transported, both the host and client need basic information about each other. This is done by having the client send a fixed length client header when it successfully connects to the host and the host will reply with it's own fixed length host header. Descriptions of these headers can be found in sections 1.4 and 1.5. 

### 1.2 MRCI Frames ###

```
[type_id][cmd_id][branch_id][data_len][payload]

type_id   - 1byte    - 8bit little endian integer type id of the payload.
cmd_id    - 2bytes   - 16bit little endian integer command id.
branch_id - 2bytes   - 16bit little endian integer branch id.
data_len  - 3bytes   - 24bit little endian integer size of the payload.
payload   - variable - the actual data to be processed.
```

A full description of the type id's can be found in the [Type_IDs.md](type_ids.md) document.

### 1.3 Versioning System ###

The host uses the typical 3 number versioning system: Major.Minor.Patch

* **Major** - this indicates any major changes to the code of the application that renders versions of different majors incompatible with each other.
* **Minor** - this indicates only minor changes to the code that may require a few conditional blocks to maintain compatibility.
* **Patch** - this indicates changes that won't require any behaviour changes at all to maintain compatibility.

Any increments to the major resets the minor and patch to 0. Any 3rd party client applications connecting to a MRCI host need to be aware of this versioning but does not need to adopt it as it's own version number.

### 1.4 Client Header ###

```
[tag][appName][coName]

tag     - 4bytes   - 0x4D, 0x52, 0x43, 0x49 (MRCI)
appName - 134bytes - UTF16LE string (padded with 0x00)
coName  - 272bytes - UTF16LE string (padded with 0x00)
```

notes:

* The **tag** is just a fixed ascii string "MRCI" that indicates to the host that the client is indeed attempting to use the MRCI protocol.

* The **appName** is the name of the client application that is connected to the host. It can also contain the client's app version if needed because it doesn't follow any particular standard. This string is accessable to all modules so the commands themselves can be made aware of what app the user is currently using.

* The **coName** is the common name of a SSL certificate that is currently installed in the host. Depending on how the host is configured, it can contain more than one installed SSL cert so coName can be used by clients as a way to request which one of the SSL certs to use during the SSL handshake.

### 1.5 Host Header ###

```
Format:

[reply][major][minor][patch][sesId]

reply - 1byte   - 8bit little endian unsigned int
major - 2bytes  - 16bit little endian unsigned int
minor - 2bytes  - 16bit little endian unsigned int
patch - 2bytes  - 16bit little endian unsigned int
sesId - 28bytes - 224bit sha3 hash
```

notes:

* **reply** is a numeric value that the host returns in it's header to communicate to the client the result of it's evaluation of the client's header.

    * reply = 1, means the client version is acceptable and it does not need to take any further action.
    * reply = 2, means the client version is acceptable but the host will now send it's Pem formatted SSL cert data in a ```HOST_CERT``` mrci frame just after sending it's header. After receiving the cert, the client will then need to send a STARTTLS signal using this cert.
    * reply = 4, means the host was unable to load the SSL cert associated with the common name sent by the client. The session will auto close at this point.

* **sesId** is the session id. It is a unique 224bit sha3 hash generated against the current date and time of session creation (down to the msec) and the machine id. This can be used by the host and client to uniquely identify the current session or past sessions.
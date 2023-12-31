# Auction Server

This project implements a simple auction platform utilizing **TCP** and **UDP** sockets. Users can open (host) an auction to sell some asset, and close it, as well list ongoing auctions and make bids.

It is composed of an **Auction Server** (_AS_) and a **User Application** (_User_). The AS and multiple User application instances are intended to operate simultaneously on different machines connected to the Internet.  \
The AS will be running on a machine with known IP address and ports.

The implementation uses the application layer protocols operating according to the client-server paradigm, using the transport layer services made available by the socket interface.

## Actions

This section explains in detail the actions that can be performed by the User application and the _AS_'s responses. It can be skipped.

The interface, using the keyboard, allows the _User_ application to do the following actions:
- <ins>**Login**</ins>: Each user is identified by a _user_ ID _UID_, which is a 6-digit number, and a _password_ composed of 8 alphanumeric characters. When the AS receives a login request it will inform of a successful or incorrect login attempt or, in case the _UID_ was not present in the AS, register a new _user_.
- <ins>**Open a new auction**</ins>: The _User_ application sends a message to the AS asking to open a new auction, providing a short description name (up to 10 alphanumerical characters), an image (or other document file) of the asset to sell, the start value (up to 6 digits), and the duration of the auction in seconds (up to 5 digits). In reply, the AS informs if the request was successful along with the assigned auction identifier, _AID_, a 3-digit number.
- <ins>**Close ongoing auction**</ins>: The _User_ application sends a message to the AS asking to close an ongoing auction, which had been started by the logged in _user_. The AS will reply informing whether the auction was successfully closed, cancelling the sale, or if the auction time had already ended.
- <ins>**List auctions started by this user**</ins> (_myauctions_): The _User_ application asks the AS for a list of the auctions started by the logged in _user_. The AS will reply with the requested list, or with information that the _user_ has not started any auction.
- <ins>**List auctions for which this user made a bid**</ins> (_mybids_): The _User_ application asks the AS for a list of the auctions in which the logged in _user_ has placed a bid. The AS will reply with the requested list, or with information that the _user_ has not made a bid in any auction.
- <ins>**List all auctions**</ins>: The _User_ application asks the AS for a list of auctions. The AS will reply with the requested list, or with information that no auctions were yet started.
- <ins>**Show asset**</ins>: The _User_ application asks the AS to send the file associated with the asset in sale for a specified auction. In reply, the AS sends the required file or an error message. The file is stored, and its name and the directory of storage are displayed to the _user_. Filenames are limited to a total of 24 alphanumerical characters (plus ‘-‘, ‘_’ and ‘.’), including the separating dot and the 3-letter extension: “_nnn…nnnn.xxx_”. The file size is limited to 10 MB, represented using a maximum of 8 digits.
- <ins>**Bid**</ins>: The _User_ application asks the AS to place a bid, with the indicated value, for the selected auction. The AS will reply reporting the result of the bid: accepted, refused (if _value_ is not larger than the previous highest bid), or informing that the auction is no longer active.
- <ins>**Show record**</ins>: The _user_ asks the AS for the record of an auction. The AS will reply with information about the auction, including its name, start value, start time and duration, followed by a description of the more recently received bids (up to 50), including the bidder ID, the value and time of the bid, as well as an indication if the auction was finished and when.
- <ins>**Logout**</ins>: The _user_ can ask the AS server to terminate the interaction (logout).
- <ins>**Unregister**</ins>: The user can ask the AS server to unregister this _user_.
- <ins>**Exit**</ins>: The user can ask to exit the _User_ application. If a _user_ is still logged in, then the _User_ application informs the _user_ that the logout command should be executed first.

## User Application (User)

This program, implementing the users of the auction platform (User), is invoked using:

```./user [-n ASIP] [-p ASport]```

where:

&nbsp;&nbsp;&nbsp;&nbsp;**_ASIP_** is the IP address of the machine where the auction server (_AS_) runs. This is an optional argument. If this argument is omitted, the _AS_ should be running on the same machine.

&nbsp;&nbsp;&nbsp;&nbsp;**_ASport_** is the well-known port (TCP and UDP) where the _AS_ accepts requests. This is an optional argument. If omitted, it assumes the value 58002.

Once the User application is running, it can open or close an auction, as well as check the status of the auctions started by this user application. The User application can ask for a list of currently active auctions, and then for a specific auction it can ask that the auction data be shown, to see the status of the bidding process, or to make a new bid. In the first login of a user its ID, _UID_, is used to register this user in the AS server. The user also has the possibility to logout, unregister or exit the User application. After unregistering, a user may register again, issuing again a login command but defining a new password. All data existing in the server about a user from previous registrations is preserved, except for the password. 

The commands supported by the User interface (using the keyboard for input) are:

- **`login`** _`UID password`_ – following this command the User application sends a message to the AS, using the UDP protocol, who confirms the UID and password of this user, or registers it if this UID is not present in the AS database.  \
The result of the request is displayed: successful login, incorrect login attempt, or new _user_ registered.
- **`logout`** – the User application sends a message to the AS, using the UDP protocol, asking to logout the currently logged in user, with ID _UID_.  \
The result of the request is displayed: successful logout, unknown _user_, or _user_ not logged in.
- **`unregister`** – the _User_ application sends a message to the AS, using the UDP protocol, asking to unregister the currently logged in _user_. A logout operation is also performed.  \
The result of the request is displayed: successful unregister, unknown _user_, or incorrect unregister attempt.
- **`exit`** – this is a request to exit the User application. If a _user_ is still logged in the User application should inform the user to first execute the logout command. Otherwise, the application can be terminated. This is a local command, not
involving communication with the AS.
- **`open`** _`name asset_fname start_value timeactive`_ – the User application establishes a TCP session with the AS and sends a message asking to open a new auction, whose short description name is _name_, providing an image of the asset to sell, stored in the file _asset_fname_, indicating the start value for the auction, _start_value_, and the duration of the auction, _timeactive_. In reply, the AS sends a message indicating whether the request was successful and the assigned auction identifier, _AID_, which is displayed to the User. After receiving the reply from the AS, the User closes the TCP connection.
- **`close`** _`AID`_ – the User application sends a message to the AS, using the TCP protocol, asking to close an ongoing auction, with identifier _AID_, that had been started by the logged in _user_. The AS will reply informing whether the auction was successfully closed, cancelling the sale, or if the auction had already ended. This information is displayed to the User. After receiving the reply from the AS, the User closes the TCP connection.
- **`myauctions`** or **`ma`** – the User application sends a message to the AS asking for a list of the auctions started by the logged in _user_. The AS will reply with the requested list, or with information that the _user_ is not involved in any auctions. This information is displayed to the User.
- **`mybids`** or **`mb`** – the User application sends a message to the AS, using the UDP protocol, asking for a list of the auctions for which the logged in _user_ has placed a bid. The AS will reply with the requested list, or with information that the user has not placed any bids. This information is displayed to the User.
- **`list`** or **`l`** – the User application sends a message to the AS, using the UDP protocol, asking for a list of auctions. The AS will reply with the requested list, or with information that no auctions were yet created. This information is displayed to the User.
- **`show_asset`** _`AID`_ or **`sa`** _`AID`_ – the User establishes a TCP session with the AS and sends a message asking to receive the file of the asset in sale for auction number _AID_. In reply, the AS sends the required file, or an error message. The file is stored and its name and the directory of storage are displayed to the User. After receiving the reply from the AS, the user closes the TCP connection.
- **`bid`** _`AID value`_ or **`b`** _`AID value`_ – the User application sends a message to the AS, using the TCP protocol, asking to place a bid for auction _AID_ of value _value_. The AS will reply reporting the result of the bid: accepted, refused (if _value_ is not larger than the previous highest bid), or informing that the auction is no longer active. The _user_ is not allowed to bid in an auction hosted by himself. This information is displayed to the User. After receiving the reply from the AS, the User closes the TCP connection.
- **`show_record`** _`AID`_ or **`sr`** _`AID`_ – the User application sends a message to the AS, using the UDP protocol, asking to see the record of auction _AID_. The AS will reply with the auction details, including a list of the more recently received bids (up to 50) and information if the auction is already closed. This information is displayed to the User.

Only one messaging command can be issued at a given time.

## Auction Server (AS)

The program implementing the Auction Server (AS) is invoked with the command:

``./AS [-p ASport] [-v]``

where:

&nbsp;&nbsp;&nbsp;&nbsp;**_ASport_** is the well-known port where the _AS_ server accepts requests, both in UDP and TCP. This is an optional argument. If omitted, it assumes the value 58002.

The AS makes available two server applications, both with well-known port _ASport_: one in UDP, used for managing the auction and the bids, and the other in TCP, used to transfer the files with asset images to the User application.  \
If the _–v_ option is set when invoking the program, it operates in _verbose_ mode, meaning that the AS outputs to the screen a short description of the received requests (_UID_, type of request) and the IP and port originating those requests.  \
Each received request starts being processed the moment it is received.

1. Dump (to chat) all words currently stored
2. Wordsets
3. Fix this bug:
> Dispatching new read.
> Parsing received data. Bytes transferred: 671
> A message was created.
> Attempting to parse command: queue skribbl
> Creating an HTTP POST request.
>> DATA: Sending request...
POST /api/v6/channels/[channel removed]/messages HTTP/1.1
Host: discordapp.com
User-Agent: Boost.Beast/266
Authorization: Bot [token removed, nice try though]
Content-Type: application/json
Content-Length: 68

{"content":"Queuing for games: \"skribbl\"! Reacting does nothing!"}
===========
> Receiving POST response.
Error: Received error: end of stream
> Encountered error while shutting down web context stream.
terminate called after throwing an instance of 'boost::system::system_error'
  what():  Broken pipe
[1]    3460 abort      ./GameBot
4. Automatically remove duplicates (case and metaword insensitive)

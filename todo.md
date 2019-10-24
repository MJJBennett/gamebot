1. Dump (to chat) all words currently stored - Done! (With wordset support)
2. Wordsets - Done!
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
5. [Mega] Add queue functionality.
6. Break up large messages (what is the max?) into smaller ones to get around max size limits.
7. Make send() obey ratelimiting procedures. This will require research into exactly what the ratelimit portion of headers indicates, and where it's located.


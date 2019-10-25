TODO (In no particular order)

1. Automatically remove duplicates (case and metaword insensitive)
2. [Mega] Add queue functionality.
3. Break up large messages (what is the max?) into smaller ones to get around max size limits.
4. Make send() obey ratelimiting procedures. This will require research into exactly what the ratelimit portion of headers indicates, and where it's located.
5. Check for out of bounds JSON access in all FileIO operations.
6. Make operations channel/guild specific.

DONE
1. Dump (to chat) all words currently stored - Done! (With wordset support)
2. Wordsets - Done!
3. Fix this bug - Done! Fixed by ignoring the error. :)
> Creating an HTTP POST request.
>> DATA: Sending request...
[Snip]
> Receiving POST response.
Error: Received error: end of stream
> Encountered error while shutting down web context stream.
terminate called after throwing an instance of 'boost::system::system_error'
  what():  Broken pipe
[1]    3460 abort      ./GameBot

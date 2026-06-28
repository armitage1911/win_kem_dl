Yet another kemono downloader but recreated out of curiosity in C89 with libcurl, yyjson, stb_sprintf and tinflate. 

I did this in VS2026 but u can also build this app with GCC (both Linux and Android) and TCC (althrought i didn't tested it, lol). 

Using: 
Just pass 'service' 'userid' postid' into an app. I.e. 'app.exe patreon 12345 12345'. 

It works like a charm but it has three downsides: 
  -- Works unstable if u using CF WARP or any VPN's. 
  -- Doesn't support async because i dunno where to implement it. 
  -- Because of kemono devs it can't download the first attachment. 

Cheers! 

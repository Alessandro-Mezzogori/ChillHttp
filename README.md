# ChillHttp

Naive implementation of tcp sockets - http comunication.
Used to explore the complexities in high troughput situations of a webserver 
while being extendible via LUA scripting and configuration

## WIP
- [ ] [TECH] Thread per request architecture backed by a threadpool ( aka task based work )
- [ ] [TECH] Cross platform ( main targets are ubuntu and windows )
- [ ] [TECH] Create a lua setup helper functions 
- [ ] Lsp lua support for ChillHttp lua structures 
- [ ] Cleaning and adding more functionalities OOB with the lua api
- [ ] Basic attack protection ( slowris and generic high load ddos )
- [ ] dynamic routing setup trough lua:
- [ ] args for static files:
	- [ ] conf.lua
	- [ ] pipeline.lua
	- [ ] serving folder
	- [ ] ecc...
- [ ] openapi support from lua

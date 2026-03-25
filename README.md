# rpi-smi

> This repo contains the code for the dissertation project, High Speed Communication Between Raspberry Pi via Secondary Memory Interface.

The project aims to create tools and document information regarding the Secondary Memory Interface.

### Todo:
- [ ] Finish full documentation 
- [ ] Debug status bits and their swapped nature
- [ ] Move all register implementation to either bitfields or shifts and not a mix of both
- [ ] Clean up smi_await hotpaths and test
- [ ] Add proper GPIO helper functionality
- [ ] Add proper return error codes for config functions
- [ ] Fix the DMA issue with PAGE_SIZE?
- [x] Test multiple DSx devices
- [x] Properly implement DMA transfers

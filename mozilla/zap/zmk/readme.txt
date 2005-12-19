Mozilla ZAP Media Kit
=====================

Built-in node types:
====================

----------------------------------------------------------------------

1) audioin 
----------

Audio source.

Sources: 1
Sinks: none

Control interfaces: zapIAudioIn

Node parameters: 
- zapIAudioDevice "device" (default: default audio in)

Source parameters:
- double "sample_rate" : sample rate in Hz (default: 8000)
- double "frame_duration" : duration of one frame in s (default: 0.02)
- unsigned long "channels" : number of channels (default: 1)
- ACString "sample_format" : "float32_1" | "float32_32768" | "int16" | "int32"
                             (default: "float32_32768")

Output stream:
audio stream with
- ACString "type" == "audio"
- double "sample_rate"
- double "frame_duration"
- unsigned long "channels"
- unsigned long "sample_format"

----------------------------------------------------------------------

2) audioout
-----------

Audio sink.

Sources: none
Sinks: 1

Control interfaces: zapIAudioOut

Node parameters: 
- zapIAudioDevice "device" (default: default audio out)

Input stream:
audio stream with
- ACString "type" (== "audio")
- double "sample_rate"
- double "frame_duration"
- unsigned long "channels"
- unsigned long "sample_format"

----------------------------------------------------------------------

3) audio-mixer
--------------

Audio mixer node, mixes n audio sinks to one audio source.

Control interfaces:
- 

Sources: 1
Sinks: n

Node parameters:
- double "sample_rate" : sample rate in Hz (default: 8000)
- double "frame_duration" : duration of one frame in s (default: 0.02)
- unsigned long "channels" : number of channels (default: 1)
- ACString "sample_format" : "float32_32768" (default: "float32_32768")

Input streams:
audio stream with
- ACString "type" (== "audio")
- double "sample_rate" (equals corresponding node parameter)
- double "frame_duration" (equals corresponding node parameter)
- unsigned long "channels" (equals corresponding node parameter)
- unsigned long "sample_format" (== "float32_32768")

Output stream:
audio stream with
- ACString "type" (== "audio")
- double "sample_rate" (equals corresponding node parameter)
- double "frame_duration" (equals corresponding node parameter)
- unsigned long "channels" (equals corresponding node parameter)
- unsigned long "sample_format" (== "float32_32768")


----------------------------------------------------------------------

4) buffer
---------

General purpose packet buffer.

Sources: 1
Sinks: 1

Node parameters:
- int "max_size" : maximum number of packets to buffer (10)
- int "prefill_size" : no of packets to prebuffer at start of stream (0)
- boolean "rebuffer" : refill buffer to prefill_size whenever the number
                       of packets in the buffer drops to 0 (true)

Input stream:
any

Output stream:
any

----------------------------------------------------------------------

5) udp-socket
-------------

A UDP socket for sending and receiving datagrams.

Control interfaces:
- zapIUDPSocket

Sources: 1
Sinks: 1

Node parameters:
- nsIUDPSocket "socket" : An initialized udp socket (nsIUDPSocket). 
                          The socket will be closed on removal from 
                          the graph.
- unsigned short "port" : local port of udp socket (default: 0)
If both "socket" and "port" are given, "port" will be ignored.
If neither "port" nor "socket" are given, the behaviour will be as 
if a "port"==0 had been specified (allocate a random free port).

Input stream:
datagram stream with
- ACString "type" (== "datagram")
Frames must implement nsIDatagram in addition to zapIMediaFrame

Output stream:
datagram stream with
- ACString "type" (== "datagram")
Frames implement nsIDatagram in addition to zapIMediaFrame


----------------------------------------------------------------------

6) udp-socket-pair
------------------

A UDP socket pair (for rtp/rtcp). port_a and port_b will be chosen
such that portbase <= port_a, port_a = 2*n, and port_b = 2*n+1

Control interfaces:
- zapIUDPSocketPair

Sources: 2
- ACString "name" == "socket-a"
- ACString "name" == "socket-b"

Sinks: 2
- ACString "name" == "socket-a"
- ACString "name" == "socket-b"

Node parameters:
- unsigned short "portbase" : port at which to start searching for a 
                              free pair (default: 49152)

Input streams:
- as in udp-socket

Output streams:
- as in udp-socket

----------------------------------------------------------------------

7) speex-encoder
----------------

Filter for encoding audio streams into speex streams.

Control interfaces: zapISpeexEncoder

Node parameters:
- double "sample_rate" : 8000|16000|32000 (default: 8000)

Input stream:
audio stream with
- ACString "type" (== "audio")
- double "sample_rate" (== 8000, 16000, or 32000; must match node parameter)
- double "frame_duration" (== 0.02)
- unsigned long "channels" (== 1)
- unsigned long "sample_format" (== "float32_32768")

Output stream:
audio/speex stream with
- ACString "type" == "audio/speex"

----------------------------------------------------------------------

8) speex-decoder
----------------

Filter for decoding speex streams as encoded using speex-encoder

Control interfaces: zapISpeexDecoder

Node parameters:
-double "sample_rate" : 8000|16000|32000 (default:8000)

Input stream:
audio/speex stream with
- ACString "type" == "audio/speex"

Output stream:
audio stream with
- ACString "type" (== "audio")
- double "sample_rate" (== 8000, 16000, or 32000; matches node parameter)
- double "frame_duration" (== 0.02)
- unsigned long "channels" (== 1)
- unsigned long "sample_format" (== "float32_32768")

----------------------------------------------------------------------

9) speex-rtp-packetizer
-----------------------

Filter for converting speex streams into rtp streams.

Node parameters:
- unsigned short "payload_type" (default: 96)

Input stream:
- ACString "type" (== "audio/speex")

Output stream:
- ACString "type" (== "rtp")
- SSRC and sequence number NOT set.

----------------------------------------------------------------------

10) speex-rtp-depacketizer
--------------------------

Filter for converting rtp streams into speex streams.

Input stream:
- ACString "type" (== "rtp")

Output stream:
- ACString "type" (== "audio/speex")

----------------------------------------------------------------------

11) g711-encoder
----------------

Filter for encoding audio streams into g.711 (pcmu or pcma) streams.

Control interfaces: 

Node parameters:
- ACString "type" == "audio/pcmu" | "audio/pcma" (default: "audio/pcmu")

Input stream:
audio stream with
- ACString "type" (== "audio")
- double "sample_rate" (== 8000)
- double "frame_duration" (== 0.02)
- unsigned long "channels" (== 1)
- unsigned long "sample_format" (== "float32_32768")

Output stream:
audio stream with
- ACString "type" == "audio/pcmu" | "audio/pcma" (as node parameter)

----------------------------------------------------------------------

12) g711-decoder
----------------

Filter for decoding g.711 streams as encoded using g711-encoder

Control interfaces: 

Node parameters:

Input stream:
audio stream with
- ACString "type" == "audio/pcmu" | "audio/pcma"

Output stream:
audio stream with
- ACString "type" (== "audio")
- double "sample_rate" (== 8000)
- double "frame_duration" (== 0.02)
- unsigned long "channels" (== 1)
- unsigned long "sample_format" (== "float32_32768")

----------------------------------------------------------------------

13) g711-rtp-packetizer
-----------------------

Filter for converting g.711 streams into rtp streams.

Node parameters:
- unsigned short "payload_type" (default: 0)

Input stream:
- ACString "type" (== "audio/pcmu" | "audio/pcma")

Output stream:
- ACString "type" (== "rtp")
- SSRC and sequence number NOT set.

----------------------------------------------------------------------

14) g711-rtp-depacketizer
-------------------------

Filter for converting rtp streams into g.711 streams.

Node parameters:
- ACString "type" == "audio/pcmu" | "audio/pcma" (default: "audio/pcmu")

Input stream:
- ACString "type" (== "rtp")

Output stream:
- ACString "type" == "audio/pcmu" | "audio/pcma" (as node parameter)

----------------------------------------------------------------------

15) rtp-session
---------------

Maintains a unicast RTP session.

Node parameters:
- ACString "address" : destination address for datagrams
- unsigned short "rtp_port" : destination port for rtp datagrams
- unsigned short "rtcp_port" : destination port for rtcp datagrams

Sinks: 3 (ACString "name" == "local-rtp"|"remote-rtp"|"remote-rtcp")
Sources: 3 (ACString "name" == "local-rtp"|"remote-rtp"|"remote-rtcp")

----------------------------------------------------------------------

16) rtp-transmitter
-------------------

Filter for transmitting RTP packets. Used internally by rtp-session.

Node parameters:
(XXX - unsigned long "SSRC" )
- ACString "address" : destination address for datagrams
- unsigned short "port" : destination port for datagrams

Input stream:
- ACString "type" (== "rtp")
rtp-transmitter will set SSRC and sequence number

Output stream:
- ACString "type" (== "datagram")
- Frames implement nsIDatagram in addition to zapIMediaFrame


----------------------------------------------------------------------

17) rtp-receiver
----------------

Filter for receiving RTP datagrams. Used internally by rtp-session.

Input stream:
- ACString "type" (== "datagram")

Output stream:
- ACString "type" (== "rtp")


----------------------------------------------------------------------

18) splitter
------------

Duplicates one input to several outputs.

Sources: n
Sinks: 1

Node parameters:

Input stream:
any

Output stream:
any


----------------------------------------------------------------------

19) rtttl-player 
----------------

Audio source for playing RTTTL-encoded ringtones.
RTTTL is the "Ringing Tones Text Transfer Language", a popular format
for Nokia mobile phone ringtones.

Sources: 1
Sinks: none

Control interfaces:

Node parameters: 
- ACString "rtttl" : The rtttl source code to be played
- boolean "loop" : true: the sample should be looped indefinitely (default: true)
- double "amplitude" : 0.0 <= amplitude <= 1.0 (default: 0.5)

Source parameters:
- double "sample_rate" : sample rate in Hz (default: 8000)
- double "frame_duration" : duration of one frame in s (default: 0.02)
- unsigned long "channels" : number of channels (default: 1)
- ACString "sample_format" : "float32_1" | "float32_32768" | "int16" | "int32"
                             (default: "float32_32768")

Output stream:
audio stream with
- ACString "type" == "audio"
- double "sample_rate"
- double "frame_duration"
- unsigned long "channels"
- unsigned long "sample_format"

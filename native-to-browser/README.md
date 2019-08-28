# Native App <=> Browser App
Basic browser app that connects to the native-webrtc peerconnection-server and plays the live stream provided by the native-webrtc peerconnection-client.

_Note: this example is a one-way only connection. Only the browser will render the video stream; not the native client. Nevertheless, this example can easily be extended to support two-way connections or rendering in the native app only._

## Getting Started

Fetch the native-webrtc sources and compile
```
# Create and enter our working directory
mkdir webrtc
cd webrtc

# Get Depot Tools
git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git
export PATH=$PATH:/path/to/depot_tools

# For Windows or for more information, see
# https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html

# Get WebRTC
# Note: This step may take an hour or two.
fetch --nohooks webrtc
cd src
git checkout branch-heads/72
gclient sync


# Build WebRTC
gn gen out/Default   # generate ninja build scripts
ninja -C out/Default # change to (-C) out/Default and build

# To add command-line build options:
# > gn gen out/Default "--args=<your args>"

# To build a single target instead of all targets:
# > ninja -C out/Default <target>
```
The hard part is done!

Now to run the demo...

1. `./peerconnection_server --port 8080`
2. `./peerconnection_client`
3. Open `index.html` in a browser.
4. Connect both `peerconnection_client` and browser ( `index.html` ) to the server.
5. Select the peer in `peerconnection_client`

## Troubleshooting

If there are no warnings or errors in the browser's log, and the native-app has enabled the webcam, but the browser is still not playing the stream, it is likely because the player was not told to start playing.

In the browser's terminal, run:
```
var remoteVideoElement = document.getElementById('remote-video');
remoteVideoElement.play()
```
Alterantively (in Chrome), right-click on the video play; click "Show controls"; And then click the play-button.

## Changes

This example is based on auscaster's "webrtc native to browser peerconnection example" project. The primary change was an upgrade in how HTTP requests were made. The `fetch()` API was used as a replacement for the former deprecated `XMLHttpRequest()`.

## Fun Facts

* WebRTC uses _'descriptions'_ to connect with peers. A server is technically not necessary to establish a connection so long as both peers are able to exchange descriptions.
* Descriptions can be sent through classic HTTP requests (like the native peerconnection example), via websockets, or even through text, email, or snail-mail.
* The method used to share WebRTC descriptions needs to be secure. This helps prevent attackers and other unwelcomed guests from invading.

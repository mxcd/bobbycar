const dgram = require("dgram");
const socket = dgram.createSocket("udp4");

const WebSocketServer = require("ws").Server;
const wss = new WebSocketServer({ port: 8090 });

const ip = "10.10.1.11"; // www.example.com
const port = 9000;

let torqueRequestLeft = 0;
let torqueRequestRight = 0;

const sendThings = () => {
  // format torque request into fixed length decimal
  const torqueRequestLeftStr = torqueRequestLeft.toFixed(2).padStart(6, "0");
  const torqueRequestRightStr = torqueRequestRight.toFixed(2).padStart(6, "0");
  const msg = `${torqueRequestLeftStr};${torqueRequestRightStr}`;
  // console.log("sending ", msg);
  socket.send(msg, port, ip);
};

wss.on("connection", function connection(ws) {
  ws.on("error", console.error);

  ws.on("message", function message(data) {
    console.log("received: %s", data);
    const parsed = JSON.parse(data);
    torqueRequestLeft = parsed.left;
    torqueRequestRight = parsed.right;
  });
});

setInterval(sendThings, 10);

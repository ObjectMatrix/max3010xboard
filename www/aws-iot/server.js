const  aws = require('aws-sdk');
const awsIot = require('aws-iot-device-sdk');
// express and socket.io
const express = require('express')
const bodyParser = require('body-parser')
const request = require('request')
const app = express()

const http = require('http').Server(app);
const io = require('socket.io')(http);


const spo2Sub = async () => {
  //setup paths to certificates
  const device = awsIot.device({
      keyPath: './private.pem.key',
      certPath: './certificate.pem.crt',
      caPath: './AmazonRootCA1.pem',
      clientId: 'spo2',
      region: 'us-east-1',
      host: 'xyz-ats.iot.us-east-1.amazonaws.com',
  });
  device.on('error', function(err) {
    console.log('Error', err);
  });
  device
    .on('connect', function() {
      device.subscribe('spo2/pub');
      });

  device
    .on('message', function(topic, payload) {
      // convert the payload to a JSON object
    try {
      var payload = JSON.parse(payload.toString());
      console.log(payload.value)
      let msg = payload.value || 80   // new Buffer.from(payload).toString()
      io.emit('spo2', msg);

    } catch(e) {
        console.log(e);
    }

      //check for TOPIC name
      if(topic == 'spo2/pub'){
         console.log('TOPIC: ', topic);
      }

    });
}

spo2Sub()

// socket.io and exptress
app.use(bodyParser.urlencoded({ extended: true }));

app.get('/', (req, res) => {
  res.sendFile('public/index.html', { root: __dirname });
})
app.get('/smoothie.js', function(req, res) {
  res.sendFile('public/smoothie.js', { root: __dirname });
})

io.on('connection', function(socket) {
  console.log('A new WebSocket connection has been established');
});

var clientId = 'mqttjs_' + Math.random().toString(16).substr(2, 8)


http.listen(4000,  () => {
  console.log('http://localhost:4000')
})

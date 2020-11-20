const express = require('express')
const bodyParser = require('body-parser')
const request = require('request')
const app = express()

const http = require('http').Server(app);
const io = require('socket.io')(http);

const mqtt = require('mqtt')
const HttpsProxyAgent = require('https-proxy-agent')

const mqttServer = 'mqtt://10.0.0.76' //mqtt
const username = '<USERNAME>';
const password = '<PASSWORD>';
const topic = '/esp32/spO2'
const client  = mqtt.connect(mqttServer)

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

client.subscribe(topic)
client.on('message', function (topic, message) {
  let msg = new Buffer.from(message).toString()
  io.emit('spo2', msg);
  console.log(msg);
})

http.listen(3000,  () => {
  console.log('http://localhost:3000')
})

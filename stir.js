const https = require('http');
const fs = require('fs');
var schedule = require('node-schedule');


console.log('Reading Config...');
var content = JSON.parse(fs.readFileSync('./config.json'));
console.log(content);

var hosts = [];

content.forEach(function(host) {
var regex = /\/sys\//gi;
host = host.replace(regex, '');
var regex = /http:\/\//gi;
host = host.replace(regex, '');
hosts.push(host);
});


function stats()
{
hosts.forEach(function(host) {

console.log("GETting from "+host+"...");
var options = {
	hostname: host,
	port:80,
	path:'/tstat',
	method: 'GET'
};

const req = https.request(options, (res) => {
	console.log(`statusCode: ${res.statusCode}`);
	res.on('data', (d) => {
			console.log(JSON.parse(d));
	});


});
req.on('error',(error) => {
	console.error(error);
});
req.end();
});
};

function fanmode(param)
{
hosts.forEach(function(host) {

var data = JSON.stringify({
	  fmode: param,
	  hold: 0 
   });
console.log("POSTing "+data+" to "+host+"...");

var options = {
	  hostname: host,
	  port: 80,
	  path: '/tstat',
	  method: 'POST',
	  headers: {
		      //'Content-Type': 'application/json',
		      'Content-Type': 'application/json',
		      'Content-Length': data.length
		    }
};

const req = https.request(options, (res) => {
	  console.log(`statusCode: ${res.statusCode}`)

	  res.on('data', (d) => {
		      process.stdout.write(d)
		    })
});

req.on('error', (error) => {
	  console.error(error)
});
req.write(data)
req.end()

});
};

var job1 = schedule.scheduleJob('0 * * * *',function(fireDate) {
	console.log(fireDate.toLocaleTimeString()+": Turning fans on...");
	fanmode(2);
});

var job2 = schedule.scheduleJob('15 * * * *',function(fireDate) {
	console.log(fireDate.toLocaleTimeString()+": Turning fans off...");
	fanmode(0);
});

var job3 = schedule.scheduleJob('*/10 * * * *',function(fireDate) {
	console.log(fireDate.toLocaleTimeString()+": Getting status...");
	stats();
});




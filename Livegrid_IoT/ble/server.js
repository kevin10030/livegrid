var http = require('http');
var fs = require('fs');

function decode(buf) {
	var StringDecoder = require('string_decoder').StringDecoder;
	var d = new StringDecoder('utf8');
	var str = d.write(buf);
	// console.log(buf); //write buffer
	// console.log(str); // write decoded buffer;
	return str;
}
  
const PORT=8080; 
var buf = new Buffer("aaa");
var str = decode(buf);//buf.toString();
// console.log(buf);
// console.log(str);

var data1 = {
  "speed" : 1,
  "scale" : 2,
  "red" : 100,
  "green" : 100,
  "blue" : 100,
  "brightness" : 0,
  "contrast" : 50
};

const data2 = require("./index.js");
data2["data"] = data1;
const data = require("./index.js");
// console.log(data["path"]);

var json_data = JSON.stringify(data["data"]);
	fs.writeFileSync('./data/data.json', json_data)
var buf =  new Buffer(json_data);      
// console.log(json_data);
// console.log(buf);

fs.readFile('./index.html', function (err, html) {

    if (err) throw err;    

    http.createServer(function(request, response) {  
        response.writeHeader(200, {"Content-Type": "text/html"});  
        response.write(html);  
        response.end();  
    }).listen(PORT);
});
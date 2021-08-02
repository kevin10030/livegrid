const util = require("util");
const bleno = require("bleno");

const Descriptor = bleno.Descriptor;
const Characteristic = bleno.Characteristic;
const data = require("../../index.js");
//JSON STUFF
const fs = require('fs')
	
class SlidersCharacteristic {

  constructor() {
    SlidersCharacteristic.super_.call(this, {
      uuid: "d7e84cb2-ff37-4afc-9ed8-5577aeb8454c",
      properties: ["read", "write", "notify"],
      descriptors: [
        new Descriptor({
          uuid: "2901",
          value: "Assign values to json"
        })
      ]
    });
  }
  
  onSubscribe(maxValueSize, updateValueCallback) {
    console.log(`Subscribed, max value size is ${maxValueSize}`);
    this.updateValueCallback = updateValueCallback;
  }

  onUnsubscribe() {
    console.log("Unsubscribed");
    this.updateValueCallback = null;
  }
  
  onReadRequest(offset, callback) {
  	console.log("ReadRequest");
    if (offset) {
      console.log("ReadRequest1");
      callback(this.RESULT_ATTR_NOT_LONG, null);
    } else {
      console.log("ReadRequest2");
      //const buf = Buffer.alloc(1);
      //buf.writeUInt8(led.readSync());
      var jsonString= JSON.stringify(data["data"]);
      console.log("Reading data on onReadRequest");
      console.log(jsonString);
      var buf =  new Buffer(jsonString);      
      callback(this.RESULT_SUCCESS, buf);
    }
  }

  onWriteRequest(data, offset, withoutResponse, callback) {
    console.log("WriteRequest");
    if(offset) {
       console.log("WriteRequest1");
      callback(this.RESULT_ATTR_NOT_LONG);
    } else if(data.length == 0) {
      console.log("WriteRequest2");
      callback(this.RESULT_INVALID_ATTRIBUTE_LENGTH);
    } else {
      console.log("WriteRequest3");
      var json_data = data.toString();
      console.log(json_data);
      console.log(data["path"]);
      //fs.writeFileSync(data["path"], json_data);
      fs.writeFileSync("/data/data.json", json_data);
      console.log("File written on onWriteRequest");
	  data["data"] = JSON.parse(json_data);
	  console.log(data["data"]);
      callback(this.RESULT_SUCCESS);
    } 
  }
};

util.inherits(SlidersCharacteristic, Characteristic);

module.exports = SlidersCharacteristic;

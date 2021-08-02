const bleno = require("bleno");
const DeviceInfoService = require("./utils/services/device-info-service");
const DataHandlerService = require("./utils/services/data-handler-service");
const deviceInfoService = new DeviceInfoService();
const dataHandlerService = new DataHandlerService();

//JSON STUFF
const fs = require('fs');
const path = '/data/data.json';

var data = {
  "boardSize":"S",
  "demo":0,
  "noiseType":0,
  "speed" : 2,
  "scale" : 2,
  "red" : 100,
  "green" : 100,
  "blue" : 100,
  "brightness" : 0,
  "contrast" : 50,
  "selectedImageIndex":0,
  "selectedVideoIndex":0,
  "textContent":"example text",
  "textFontSize":1,
  "textFontType":1,
  "textRed":100,
  "textGreen":100,
  "textBlue":100
};

		
fs.access(path, fs.F_OK, (err) => {
	if (err) {
		var json_data = JSON.stringify(data);
		fs.writeFileSync(path, json_data)
		// console.log("File written on index");
		return
	}
	//file exists
	data = fs.readFileSync(path);
	data = JSON.parse(data);
	// console.log(data);
});


bleno.on("stateChange", state => {
  console.log(`on -> stateChange: ${state}`);

  if (state === "poweredOn") {
    bleno.startAdvertising("balenaBLE", [
      deviceInfoService.uuid,
      dataHandlerService.uuid
    ]);
  } else {
    bleno.stopAdvertising();
  }
});

bleno.on("advertisingStart", error => {
  console.log(
    `on -> advertisingStart: ${error ? "error " + error : "success"}`
  );

  if (!error) {
    bleno.setServices([deviceInfoService, dataHandlerService], error => {
      console.log(`setServices: ${error ? "error " + error : "success"}`);
    });
  }
});


module.exports.data = data;
module.exports.path = path;

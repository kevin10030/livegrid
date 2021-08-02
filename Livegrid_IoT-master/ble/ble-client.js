class BalenaBLE {
  constructor() {
    this.device = null;
    this.sliders = null;
    this.cpuVendor = null;
    this.cpuSpeed = null;
    this.onDisconnected = this.onDisconnected.bind(this);    
  }

  /* the LED characteristic providing on/off capabilities */
  async setSlidersCharacteristic() {
    const service = await this.device.gatt.getPrimaryService(0xfff0);
    const characteristic = await service.getCharacteristic(
      "d7e84cb2-ff37-4afc-9ed8-5577aeb8454c"
    );
    // characteristic.startNotifications();
    this.sliders = characteristic;

    // await this.sliders.startNotifications();

    // this.sliders.addEventListener(
    //   "characteristicvaluechanged",
    //   handleLedStatusChanged
    // );
  }

  /* the Device characteristic providing CPU information */
  async setDeviceCharacteristic() {
    const service = await this.device.gatt.getPrimaryService(0xfff1);
    const vendor = await service.getCharacteristic(
      "d7e84cb2-ff37-4afc-9ed8-5577aeb84542"
    );
    this.cpuVendor = vendor;

    const speed = await service.getCharacteristic(
      "d7e84cb2-ff37-4afc-9ed8-5577aeb84541"
    );
    this.cpuSpeed = speed;
  }

  /* request connection to a BalenaBLE device */
  async request() {
    let options = {
      filters: [
        {
          name: "balenaBLE"
        }
      ],
      optionalServices: [0xfff0, 0xfff1]
    };
    if (navigator.bluetooth == undefined) {
      alert("Sorry, Your device does not support Web BLE!");
      return;
    }
    this.device = await navigator.bluetooth.requestDevice(options);
    if (!this.device) {
      throw "No device selected";
    }
    this.device.addEventListener("gattserverdisconnected", this.onDisconnected);
  }

  /* connect to device */
  async connect() {
    if (!this.device) {
      return Promise.reject("Device is not connected.");
    }
    await this.device.gatt.connect();
  }

  /* read CPU manufacturer */
  async readCPUVendor() {
    let vendor = await this.cpuVendor.readValue();
    return decode(vendor);
  }

  /* read CPU speed */
  async readCPUSpeed() {
    let speed = await this.cpuSpeed.readValue();
    return decode(speed);
  }
 
  async readJson() {
  	 console.log("File read");
 	 let json = await this.sliders.readValue();
 	 return decode(json);
  }
  
  /* change slider values*/
  async writeJson(data) {
  	console.log("File write");
  	// console.log(data);
  	
  	//var buf = new Buffer(data);
  	var buf = new TextEncoder().encode(data);
  	await this.sliders.writeValue(buf);
    //await this.readJson();
  }

  async hexToRgb(hex) {
    const r = parseInt(hex.substring(1, 3), 16); //start at 1 to avoid #
    const g = parseInt(hex.substring(3, 5), 16);
    const b = parseInt(hex.substring(5, 7), 16);
  
    return [r, g, b];
  }

  /* disconnect from peripheral */
  disconnect() {
    if (!this.device) {
      return Promise.reject("Device is not connected.");
    }
    return this.device.gatt.disconnect();
  }

  /* handler to run when device successfully disconnects */
  onDisconnected() {
    alert("Device is disconnected.");
    location.reload();
  }
}

var util = require("util")

var bleno = require("bleno")
var BlenoPrimaryService = bleno.PrimaryService;

var SlidersCharacteristic = require("../characteristics/sliders-characteristic");

function DataHandlerService() {
    DataHandlerService.super_.call(this, {
        uuid: "fff0",
        characteristics: [new SlidersCharacteristic()]
    });
}

util.inherits(DataHandlerService, BlenoPrimaryService);

module.exports = DataHandlerService;
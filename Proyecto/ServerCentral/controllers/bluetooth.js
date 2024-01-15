// controllers/bluetooth.js
const fs = require('fs');
const path = require('path');

const filePath = path.join("./JSON", 'bluetooth.json');

exports.registerDevice = async (req, res, next) => {
    let device = req.body.name;
    console.log(req.body)

    fs.readFile(filePath, 'utf8', (err, data) => {
        if (err) {
            console.error(`Error reading file: ${err}`);
            return;
        }
        let jsonData;
        try {
            // Parse the JSON data
            jsonData = JSON.parse(data);
        } catch (parseError) {
            console.error(`Error parsing JSON: ${parseError}`);
            return;
        }
        if (!jsonData.hasOwnProperty('name')) {
            // Add the 'name' property with value 'abc'
            jsonData.name = device;

            // Convert the JSON object back to a string
            const updatedJson = JSON.stringify(jsonData, null, 2);

            // Write the updated JSON back to the file
            fs.writeFile(filePath, updatedJson, 'utf8', (writeErr) => {
                if (writeErr) {
                    console.error(`Error writing file: ${writeErr}`);
                } else {
                    console.log('Property "name" added with value ' + device);
                }
            });
        }
        else {
            console.log('Property "name" already exists');
        }
    });
    res.end()
};

exports.getDevices = async (req, res, next) => {
    fs.readFile(filePath, 'utf8', (err, data) => {
        if (err) {
            console.error(`Error reading file: ${err}`);
            return;
        }
        let jsonData;
        try {
            // Parse the JSON data
            jsonData = JSON.parse(data);
            res.json(jsonData);
            res.end()
        } catch (parseError) {
            console.error(`Error parsing JSON: ${parseError}`);
            return;
        }
    });
};

exports.deleteDevice = async (req, res, next) => {
    let name = req.body.name
    fs.readFile(filePath, 'utf8', (err, data) => {
        if (err) {
            console.error(`Error reading file: ${err}`);
            return;
        }
        let jsonData;
        try {
            // Parse the JSON data
            jsonData = JSON.parse(data);
        } catch (parseError) {
            console.error(`Error parsing JSON: ${parseError}`);
            return;
        }
        if (jsonData.hasOwnProperty('name')) {
            // Delete the 'name' property
            delete jsonData.name;

            // Convert the JSON object back to a string
            const updatedJson = JSON.stringify(jsonData, null, 2);

            // Write the updated JSON back to the file
            fs.writeFile(filePath, updatedJson, 'utf8', (writeErr) => {
                if (writeErr) {
                    console.error(`Error writing file: ${writeErr}`);
                } else {
                    console.log('Property "name" deleted');
                }
            });
        } else {
            console.log('Property "name" does not exist');
        }
    });
    res.end()
}
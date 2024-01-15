// controllers/atHome.js
const fs = require('fs');
const path = require('path');

const filePath = path.join("./JSON", 'atHome.json');

exports.registerEnter = async (req, res, next) => {
    let time = req.body.time
    let name = req.body.name
    fs.readFile(filePath, 'utf8', (err, data) => {
        if (err) {
            console.error(`Error reading file: ${err}`);
            return;
        }

        let atHome;
        try {
            // Parse the JSON data
            atHome = JSON.parse(data);
        } catch (parseError) {
            console.error(`Error parsing JSON: ${parseError}`);
            return;
        }

        if (atHome.length > 0) {
            // Find the index of the record
            const lastRecordWithTargetName = atHome.slice().reverse().find(cat => cat.name === name);

            lastRecordWithTargetName.enter = time

            const updatedJson = JSON.stringify(atHome, null, 2);

            // Write the updated JSON back to the file
            fs.writeFile(filePath, updatedJson, 'utf8', (writeErr) => {
                if (writeErr) {
                    console.error(`Error writing file: ${writeErr}`);
                } else {
                    console.log('Record enter created succesfully');
                }
            });
        }
    });
    res.end()
}


exports.registerExit = async (req, res, next) => {
    let time = req.body.time
    let name = req.body.name

    fs.readFile(filePath, 'utf8', (err, data) => {
        if (err) {
            console.error(`Error reading file: ${err}`);
            return;
        }

        let atHome;
        try {
            // Parse the JSON data
            atHome = JSON.parse(data);
        } catch (parseError) {
            console.error(`Error parsing JSON: ${parseError}`);
            return;
        }

        const newRecord = {
            "exit": time,
            //"enter": 
            "name": name,
        };

        atHome.push(newRecord);

        // Convert the updated array back to a string
        const updatedJson = JSON.stringify(atHome, null, 2);

        // Write the updated JSON back to the file
        fs.writeFile(filePath, updatedJson, 'utf8', (writeErr) => {
            if (writeErr) {
                console.error(`Error writing file: ${writeErr}`);
            } else {
                console.log('Record exit created succesfully');
            }
        });
    });
    res.end()
}

exports.getRegister = async (req, res, next) => {
    fs.readFile(filePath, 'utf8', (err, data) => {
        if (err) {
            console.error(`Error reading file: ${err}`);
            return;
        }

        let atHome;
        try {
            // Parse the JSON data
            atHome = JSON.parse(data);
            res.json(atHome)
            res.end()
        } catch (parseError) {
            console.error(`Error parsing JSON: ${parseError}`);
            return;
        }
    });
}

exports.getLastRegister = async (req, res, next) => {
    let name = req.body.name
    fs.readFile(filePath, 'utf8', (err, data) => {
        if (err) {
            console.error(`Error reading file: ${err}`);
            return;
        }

        let atHome;
        try {
            // Parse the JSON data
            atHome = JSON.parse(data);
        } catch (parseError) {
            console.error(`Error parsing JSON: ${parseError}`);
            return;
        }

        if (atHome.length > 0) {
            // Find the index of the record
            const lastRecordWithTargetName = atHome.slice().reverse().find(cat => cat.name === name);
            res.json(lastRecordWithTargetName);
            res.end();
        }
        else {
            res.end()
        }
    });

}
// controllers/cats.js
const fs = require('fs');
const path = require('path');

const filePath = path.join("./JSON", 'cats.json');

exports.getCats = async (req, res, next) => {
    // Read the JSON file
    fs.readFile(filePath, 'utf8', (err, data) => {
        if (err) {
            console.error(`Error reading file: ${err}`);
            return;
        }

        let cats;
        try {
            // Parse the JSON data
            cats = JSON.parse(data);
            res.json(cats)
            res.end()
        } catch (parseError) {
            console.error(`Error parsing JSON: ${parseError}`);
            return;
        }
    });
};

exports.addCat = async (req, res, next) => {
    let cat = req.body.name;
    let rfid = req.body.rfid;

    fs.readFile(filePath, 'utf8', (err, data) => {
        if (err) {
            console.error(`Error reading file: ${err}`);
            return;
        }

        let cats;
        try {
            // Parse the JSON data
            cats = JSON.parse(data);
        } catch (parseError) {
            console.error(`Error parsing JSON: ${parseError}`);
            return;
        }

        // Add a new cat
        const newCat = {
            "name": cat,
            "rfid": rfid.map(Number)
        };

        // Add the new cat to the array
        cats.pets.push(newCat);

        // Convert the updated array back to a string
        const updatedJson = JSON.stringify(cats, null, 2);

        // Write the updated JSON back to the file
        fs.writeFile(filePath, updatedJson, 'utf8', (writeErr) => {
            if (writeErr) {
                console.error(`Error writing file: ${writeErr}`);
            } else {
                console.log('New cat added successfully');
            }
        });
    });
    res.end()
};

exports.deleteCat = async (req, res, next) => {
    let name = req.body.name
    fs.readFile(filePath, 'utf8', (err, data) => {
        if (err) {
            console.error(`Error reading file: ${err}`);
            return;
        }

        let cats;
        try {
            // Parse the JSON data
            cats = JSON.parse(data);
        } catch (parseError) {
            console.error(`Error parsing JSON: ${parseError}`);
            return;
        }

        // Find the index of the cat with the specified name
        const catIndex = cats.pets.findIndex(cat => cat.name === name);

        // Check if the cat was found
        if (catIndex !== -1) {
            // Remove the cat from the array
            cats.pets.splice(catIndex, 1);

            // Convert the updated array back to a string
            const updatedJson = JSON.stringify(cats, null, 2);

            // Write the updated JSON back to the file
            fs.writeFile(filePath, updatedJson, 'utf8', (writeErr) => {
                if (writeErr) {
                    console.error(`Error writing file: ${writeErr}`);
                } else {
                    console.log('Cat deleted successfully');
                }
            });
        } else {
            console.log('Cat not found');
        }
    });
    res.end()
};

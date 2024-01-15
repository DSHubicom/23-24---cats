// controllers/feeding.js
const fs = require('fs');
const path = require('path');

const filePath = path.join("./JSON", 'feeding.json');

exports.getLastFeedingTime = async (req, res, next) => {
    let name = req.query.name;

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
        } catch (parseError) {
            console.error(`Error parsing JSON: ${parseError}`);
            return;
        }

        const index = cats.findIndex(item => item.name === name);

        try {
            actualTime = Math.floor(new Date().getTime() / 1000) //Hora actual
            if (index != -1) { //Si existe el animal
                if (actualTime - cats[index].time >= 4 * 60 * 60) { //Se comprueba si han pasado 4 horas
                    cats[index].time = actualTime //Si han pasado, se dice que sí y se cambia el tiempo de comida a ahora
                    res.json({ "canIt": true });
                }
                else {
                    res.json({ "canIt": false }); //Si no, se dice que no
                }
            } else { //Si no existe el animal
                const newCat = { //Se crea su hora de comida con la actual
                    "name": name,
                    "time": actualTime
                };
                cats.push(newCat)
                res.json({ "canIt": true });
                res.end()
            }
        } catch (parseError) {
            console.error(`Error handling result: ${parseError}`);
            return;
        }
        const updatedJson = JSON.stringify(cats, null, 2);

        // Write the updated JSON back to the file
        fs.writeFile(filePath, updatedJson, 'utf8', (writeErr) => {
            if (writeErr) {
                console.error(`Error writing file: ${writeErr}`);
            } else {
                console.log('New feeding time added successfully');
            }
        });
    });
};
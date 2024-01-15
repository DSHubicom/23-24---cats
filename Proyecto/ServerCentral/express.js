const express = require('express');

const app = express();

const bodyParser = require('body-parser');

port = 80

app.use(bodyParser.urlencoded({ extended: false }));

app.use(bodyParser.json());

// Centralized error handling middleware
app.use((err, req, res, next) => {
    console.error(err.stack);
    res.status(500).send('Something went wrong!');
});

// Define a router
const router = express.Router();

// Bluetooth endpoints
router.post('/bluetooth', require('./controllers/bluetooth').registerDevice);
router.get('/bluetooth', require('./controllers/bluetooth').getDevices);
router.delete('/bluetooth', require('./controllers/bluetooth').deleteDevice);

// Cats endpoints
router.get('/pets', require('./controllers/cats').getCats);
router.post('/pets', require('./controllers/cats').addCat);
router.delete('/pets', require('./controllers/cats').deleteCat);

//atHome endpoints
router.post('/enter', require('./controllers/atHome').registerEnter);
router.post('/exit', require('./controllers/atHome').registerExit);
router.get('/record', require('./controllers/atHome').getRegister);
router.get('/lastrecord', require('./controllers/atHome').getLastRegister);

router.get('/feeding', require('./controllers/feeding').getLastFeedingTime);

app.use('/', router);

// Start the server
app.listen(port, () => {
    console.log(`Server is running on port ${port}`);
});
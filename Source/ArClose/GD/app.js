
/* 
Google Drive API:
Demonstration to:
1. upload 
2. delete 
3. create public URL of a file.
required npm package: googleapis
*/
const { google } = require('googleapis');
const path = require('path');
const fs = require('fs');

const CLIENT_ID = '971361358402-3cioqihh9ts6b8itcq6hb6g8trm5ciui.apps.googleusercontent.com';
const CLIENT_SECRET = 'iELm33tmXXS8Pirjm64ZgfVg';
const REDIRECT_URI = 'https://developers.google.com/oauthplayground';

const REFRESH_TOKEN = '1//04d5hht5uPRlDCgYIARAAGAQSNwF-L9IrQowOfBsghehjdkMBcif0REP7pvowWcPEkGdxSLLiobN4OHqOEdEi7MgKetxu7KB6FMI';

const oauth2Client = new google.auth.OAuth2(
    CLIENT_ID,
    CLIENT_SECRET,
    REDIRECT_URI
);

oauth2Client.setCredentials({ refresh_token: REFRESH_TOKEN });

const drive = google.drive({
    version: 'v3',
    auth: oauth2Client,
});

/* 
filepath which needs to be uploaded
Note: Assumes example.jpg file is in root directory, 
though this can be any filePath
*/
const filePath = path.join(__dirname, 'image.jpg');

async function uploadFile() {
    try {
        const response = await drive.files.create({
            requestBody: {
                name: 'imageeee.jpg', //This can be name of your choice
                mimeType: 'image/jpg',
            },
            media: {
                mimeType: 'image/jpg',
                body: fs.createReadStream(filePath),
            },
        });

        console.log(response.data);
        console.log(response.data.id)
        const id = response.data.id
        generatePublicUrl(id)
    } catch (error) {
        console.log(error.message);
    }
    
}

uploadFile();

async function deleteFile() {
    try {
        const response = await drive.files.delete({
            fileId: 'YOUR FILE ID',
        });
        console.log(response.data, response.status);
    } catch (error) {
        console.log(error.message);
    }
}

// deleteFile();

async function generatePublicUrl(fileId) {
    try {
        //const fileId = '1MeAYPm6GrIMnIpq_AsFEvkxhBOG9fTfd';
        //const fileId = id;
        await drive.permissions.create({
            fileId: fileId,
            requestBody: {
                role: 'reader',
                type: 'anyone',
            },
        });

        /* 
        webViewLink: View the file in browser
        webContentLink: Direct download link 
        */
        const result = await drive.files.get({
            fileId: fileId,
            fields: 'webViewLink, webContentLink',
        });
        console.log(result.data.webContentLink);
        fs.writeFileSync("result.txt", result.data.webContentLink)
    } catch (error) {
        fs.writeFileSync("bad.txt", 'Govno')
        console.log(error.message);
    }
}

//generatePublicUrl();

const socket = new WebSocket("ws://localhost");

socket.onopen = function(event) {
    socket.send("IILWRSAIH");
}

fetch("/post_test", {
    method: 'POST',
    headers: {
        'Content-Type': 'text/plain',
    },
    body: "This is a example string I am sending the body of a post request."
})
.then(response => {
    if (!response.ok) {
        throw new Error('Network response was not ok');
    }
    return response.text();
})
.then(data => {
    console.log('Success:', data);
})
.catch(error => {
    console.error('Error:', error);
});
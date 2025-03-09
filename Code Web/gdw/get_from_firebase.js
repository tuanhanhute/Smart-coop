'use strict'
import firebaseConfig from "./firebase.js";

firebase.initializeApp(firebaseConfig);

const database = firebase.database();

//const access_manager = document.getElementById("access_manager");
const btnOn = document.getElementById("fan-on");
const btnOff = document.getElementById("fan-off");
const btnOpen = document.getElementById("open");
const btnClose = document.getElementById("close");
const toggleButton = document.getElementById("toggle-interaction");
const device = document.getElementsByClassName("device");
const Feeding_List = document.getElementById("Feeding_List");
const btnOn2 = document.getElementById("heater-on");
const btnOff2 = document.getElementById("heater-off");
const log = document.getElementById("log");
const Temp_color = document.getElementById("temp-color");
const Hum_color = document.getElementById("hum-color");
const PumpOn = document.getElementById("pump-on");
const PumpOff = document.getElementById("pump-off");
let Feeding_Timer_Arr;
let mode;
let heater_val = null;
let fan_val = null;
let feeding_val = null;
let drinking_val = null;
let chicken_problem = null;
let egg_out = null;


function updateRealTimeClock() {
    const realTimeClock = document.getElementById("real-time-clock");
    const now = new Date();
    const hour = now.getHours();
    const minute = now.getMinutes();
    const second = now.getSeconds();
    const day = now.getDate();
    const month = now.getMonth() + 1;
    const year = now.getFullYear();

    const formattedHours = hour < 10 ? `0${second}` : hour;
    const formattedMinutes = minute < 10 ? `0${minute}` : minute;
    const formattedSeconds = second < 10 ? `0${second}` : second;
    const formattedDays = day < 10 ? `0${day}` : day;
    const formattedMonths = month < 10 ? `0${month}` : month;

    const formattedTime =
        `<p>${formattedDays}/${formattedMonths}/${year}</p>
    <p>${formattedHours}:${formattedMinutes}:${formattedSeconds}</p>`;

    realTimeClock.innerHTML = formattedTime;

    return `[${formattedDays}/${formattedMonths}/${year} ${formattedHours}:${formattedMinutes}:${formattedSeconds}]`;

}


// Periodic time check for feeding activation
setInterval(() => {
    let time = updateRealTimeClock();
    let formatted_time = time.slice(12, 17);
    let formatted_seconds = time.slice(18, 20);

    // Check if the current time matches any in Feeding_Timer_Arr
    if (Feeding_Timer_Arr && Feeding_Timer_Arr.includes(formatted_time)) {
        if (formatted_seconds == "00") {
            database.ref("/doan1/controller/feeding").update({
                "state": "ON",
                "user": "hệ thống tự động",
                "time": time,
            });
        }
        if (formatted_seconds == "10") {
            database.ref("/doan1/controller/feeding").update({
                "state": "OFF",
                "user": "hệ thống tự động",
                "time": time,
            });
        }
    }
}, 1000); // Check every second

export default async function get_value() {
    database.ref("/doan1/coop/chicken/").on("value", function (snapshot) {
        let chicken = snapshot.child('problem').val();
        let time = snapshot.child('time').val();
        if (chicken_problem != chicken) {
            if (chicken == "NO CHICKEN") {
                log.innerText += `${time} Không có gà đang đi bộ\n`;
                chicken_problem = "NO CHICKEN";
            }
            if (chicken == "YES") {
                log.innerText += `${time} Gà không di chuyển\n`;
                chicken_problem = "YES";
                alert("Cảnh báo: Gà không di chuyển");
            }
            if (chicken == "NO") {
                chicken_problem = "NO";
            }
        }
    });

    database.ref("/doan1/coop/egg/").on("value", function (snapshot) {
        let egg = snapshot.child('out').val();
        let time = snapshot.child('time').val();
        if (egg_out != egg) {
            if (egg == "NO EGG") {
                log.innerText += `${time} Không có trứng trong chuồng\n`;
                egg_out = "NO EGG";
            }
            if (egg == "NO NEST") {
                log.innerText += `${time} Không có tổ trứng trong chuồng\n`;
                egg_out = "NO NEST";
            }
            if (egg == "YES") {
                egg_out = "YES"
                log.innerText += `${time} Trứng rơi ra khỏi tổ\n`;
                alert("Cảnh báo: Trứng rơi ra khỏi tổ");
            }
            if (egg == "NO") {
                egg_out = "NO";
            }
        }
    });

    database.ref("/doan1/controller/heater").on("value", function (snapshot) {
        let state = snapshot.child('state').val();
        let user = snapshot.child('user').val();
        let realtime = snapshot.child('time').val();
        if (heater_val != state) {
            if (state == "ON") {
                heater_val = state;
                document.getElementById("heater").src = "https://cdn-icons-png.flaticon.com/512/566/566461.png"
                log.innerText += `${realtime} Người dùng ${user} đã bật đèn sưởi\n`;
            }
            else {
                heater_val = state;
                document.getElementById("heater").src = "https://cdn-icons-png.flaticon.com/512/566/566359.png"
                log.innerText += `${realtime} Người dùng ${user} đã tắt đèn sưởi\n`;
            }
        }
    });

    database.ref("/doan1/controller/temperature").on("value", function (snapshot) {
        let temp = snapshot.val();
        document.getElementById("temp").innerHTML = temp;
        if (temp > 28) {
            Temp_color.style.color = "red";
            // alert("Nhiệt độ đã vượt ngưỡng cho phép");
        }
        else {
            Temp_color.style.color = "black";
        }
    });

    database.ref("/doan1/controller/humidity").on("value", function (snapshot) {
        let hum = snapshot.val();
        document.getElementById("humid").innerHTML = hum;
        if (hum > 70) {
            Hum_color.style.color = "red";
            //alert("Độ ẩm đã vượt ngưỡng cho phép");
        }
        else {
            Hum_color.style.color = "black";
        }
    });

    database.ref("/doan1/controller/fan").on("value", function (snapshot) {
        let state = snapshot.child('state').val();
        let user = snapshot.child('user').val();
        let realtime = snapshot.child('time').val();
        if (fan_val != state) {
            if (state == "ON") {
                fan_val = state;
                document.getElementById("fan").src = "https://cdn-icons-png.flaticon.com/512/5754/5754434.png";
                log.innerText += `${realtime} Người dùng ${user} đã bật quạt\n`;
            }
            else {
                fan_val = state;
                document.getElementById("fan").src = "https://cdn-icons-png.flaticon.com/512/5324/5324839.png";
                log.innerText += `${realtime} Người dùng ${user} đã tắt quạt\n`;
            }
        }
    });
    database.ref("/doan1/log/login").on("value", function (snapshot) {

        let user = snapshot.child('email').val();
        let realtime = snapshot.child('time').val();

        log.innerText += `${realtime} Người dùng ${user} đã đăng nhập\n`;

    });
    database.ref("/doan1/log/logout").on("value", function (snapshot) {
        let user = snapshot.child('email').val();
        let realtime = snapshot.child('time').val();

        log.innerText += `${realtime} Người dùng ${user} đã đăng xuất\n`;

    });
    database.ref("/doan1/controller/feeding").on("value", function (snapshot) {
        let state = snapshot.child('state').val();
        let user = snapshot.child('user').val();
        let realtime = snapshot.child('time').val();
        if (feeding_val != state) {
            if (state == "ON") {
                feeding_val = state;
                document.getElementById("State").src = "https://cdn-icons-png.flaticon.com/128/2855/2855629.png";
                log.innerText += `${realtime} Người dùng ${user} đã cho gà ăn\n`;
            }
            else {
                feeding_val = state;
                document.getElementById("State").src = "https://cdn-icons-png.flaticon.com/128/7128/7128634.png"
                log.innerText += `${realtime} Người dùng ${user} đã ngưng cho gà ăn\n`;
            }
        }
    });
    database.ref("/doan1/controller/drinking").on("value", function (snapshot) {
        let state = snapshot.child('state').val();
        let user = snapshot.child('user').val();
        let realtime = snapshot.child('time').val();
        if (state == "ON") {
            drinking_val = state;
            document.getElementById("StatePump").src = "https://cdn-icons-png.flaticon.com/128/12405/12405847.png"
            log.innerText += `${realtime} Người dùng ${user} đã cho gà uống\n`;
        }
        else {
            drinking_val = state;
            document.getElementById("StatePump").src = "https://cdn-icons-png.flaticon.com/128/2372/2372600.png"
            log.innerText += `${realtime} Người dùng ${user} đã ngưng cho gà uống\n`;
        }
    });
    database.ref("/doan1/controller/mode").on("value", function (snapshot) {
        mode = snapshot.child('state').val();
        let user = snapshot.child('user').val();
        let realtime = snapshot.child('time').val();
        if (mode === "auto") {
            toggleButton.innerHTML = "User";
            btnOn.disabled = true;
            btnOff.disabled = true;
            btnOn2.disabled = true;
            btnOff2.disabled = true;
            btnOpen.disabled = true;
            btnClose.disabled = true;
            PumpOn.disabled = true;
            PumpOff.disabled = true;


            alert("Đang ở chế độ điều khiển tự động");
            log.innerText += `${realtime} Người dùng ${user} đã chuyển sang điều khiển tự động\n`;
            for (let i = 0; i < device.length; i++) {
                device[i].style.opacity = "0.5";
            }
        } else {
            toggleButton.innerHTML = "Auto";
            btnOn.disabled = false;
            btnOff.disabled = false;
            btnOn2.disabled = false;
            btnOff2.disabled = false;
            btnOpen.disabled = false;
            btnClose.disabled = false;
            PumpOn.disabled = false;
            PumpOff.disabled = false;

            alert("Đang ở chế độ điều khiển thủ công");
            log.innerText += `${realtime} Người dùng ${user} đã chuyển sang điều khiển thủ công\n`;
            for (let i = 0; i < device.length; i++) {
                device[i].style.opacity = "1";
            }
        }
    });
    // Hàm này sẽ tạo danh sách cho Feeding
    database.ref("/doan1/controller/Feeding_Timer").on("value", function (snapshot) {
        Feeding_Timer_Arr = snapshot.val();

        // Xóa nội dung hiện tại
        Feeding_List.innerHTML = "";

        for (let i in Feeding_Timer_Arr) {
            let div = document.createElement('div');
            div.id = `Feeding_Timer_Div_${i}`;

            let span = document.createElement('span');
            span.innerHTML = Feeding_Timer_Arr[i];

            let button = document.createElement('button');
            button.innerHTML = "Delete";
            // Thêm sự kiện click cho button để xóa phần tử
            button.addEventListener("click", function () {
                Delete_List_Element(i, 'Feed');
            });

            div.appendChild(span);
            div.appendChild(button);
            Feeding_List.appendChild(div);
        }
    });
    /*
        if (Feeding_Timer_Arr.indexOf(formatted_time)) {
            if (formatted_seconds == "00") {
                database.ref("/doan1/controller/feeding").update({
                    "state": "ON",
                    "user": "hệ thống tự động",
                    "time": time
                });
            }
            if (formatted_seconds == "10") {
                database.ref("/doan1/controller/feeding").update({
                    "state": "OFF",
                    "user": "hệ thống tự động",
                    "time": time
                });
            }
        }*/

}

/*
function Delete_List_Element(i, which) {
    if (which == "Feed") {
        let element = document.getElementById(`Feeding_Timer_Div_${i}`);
        element.innerHTML = "";
        Feeding_Timer_Arr.splice(i, 1);
        //console.log(Feeding_Timer_Arr);
        database.ref("/doan1/controller").update({
            "Feeding_Timer": Feeding_Timer_Arr,
        });
    }
}*/

function Delete_List_Element(i, which) {
    if (which === "Feed") {
        let element = document.getElementById(`Feeding_Timer_Div_${i}`);
        if (element) {
            element.remove();
        }
        Feeding_Timer_Arr.splice(i, 1);
        database.ref("/doan1/controller").update({
            Feeding_Timer: Feeding_Timer_Arr,
        });
    }
}
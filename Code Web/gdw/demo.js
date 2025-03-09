import get_value from "./get_from_firebase.js";

const database = firebase.database();
const btnOn = document.getElementById("fan-on");
const btnOff = document.getElementById("fan-off");
const btnOpen = document.getElementById("open");
const btnClose = document.getElementById("close");
const toggleButton = document.getElementById("toggle-interaction");
const device = document.getElementsByClassName("device");
const btnOn2 = document.getElementById("heater-on");
const btnOff2 = document.getElementById("heater-off");
const PumpOn = document.getElementById("pump-on");
const PumpOff = document.getElementById("pump-off");
const submit_link = document.getElementById("submit-link");
const save_log = document.getElementById("save_log");
const copy_log = document.getElementById("copy_log");
const Feeding_Timer_Btn = document.getElementById("Feeding_Timer_Btn");
const Feeding_List = document.getElementById("Feeding_List");
let Feeding_Timer_Arr;
let mode;


await get_value();

let user_email = null;

firebase.auth().onAuthStateChanged((current_user) => {
  if (current_user) {
    user_email = current_user.email;
    console.log("Current User Email:", current_user.email);
  }
  else {
    console.log("no user");
  }
});

copy_log.addEventListener('click', function () {
  // Get the text field
  let content = document.getElementById("log").textContent;

  // Replace escaped "\n" with actual new lines for correct formatting
  const formattedContent = content.replaceAll("[", "\n[");

  try {
    navigator.clipboard.writeText(formattedContent);
    alert("Đã sao chép thành công!");
  } catch (err) {
    alert("Lỗi sao chép thất bại: " + err);
  }
});

save_log.addEventListener('click', function () {
  // Get the content of the div
  const content = document.getElementById('log').textContent;

  // Replace escaped "\n" with actual new lines for correct formatting
  const formattedContent = content.replaceAll("[", "\n[");

  // Create a Blob with the content
  const blob = new Blob([formattedContent], { type: 'text/plain' });


  // Create a temporary anchor element for downloading
  const a = document.createElement('a');

  let time = updateRealTimeClock();
  a.href = URL.createObjectURL(blob);
  a.download = `log_at_${time}.txt`; // File name

  // Trigger the download
  a.click();

  // Clean up
  URL.revokeObjectURL(a.href);
});

submit_link.addEventListener("click", function () {
  let link = document.getElementById("camera-link").value.slice(32, 43);
  console.log(link);

  if (link.length != 11) {
    alert("Vui lòng nhập link đúng https://youtube.com/watch?v=...")
  }
  database.ref("/doan1/controller").update({
    "camera_link_youtube_id": link
  });

  document.getElementById("camera").src = "https://www.youtube.com/embed/" + link;
  console.log(document.getElementById("camera").src);
})


function Add_Timer(selectedHour, selectedMinute, which) {


  if (selectedMinute < 10) {
    selectedMinute = '0' + selectedMinute;
  }

  if (selectedHour < 10) {
    selectedHour = '0' + selectedHour;
  }

  if (which == "Feed") {
    if (!Feeding_Timer_Arr) {
      Feeding_Timer_Arr = [];
    }

    if (!Feeding_Timer_Arr.includes(`${selectedHour}:${selectedMinute}`)) {
      Feeding_Timer_Arr.push(`${selectedHour}:${selectedMinute}`);

      database.ref("/doan1/controller").update({
        "Feeding_Timer": Feeding_Timer_Arr
      });
    }
  }

}

toggleButton.addEventListener("click", function () {
  let time = updateRealTimeClock();

  if (toggleButton.innerHTML === "User") {
    toggleButton.innerHTML = "Auto";
    btnOn.disabled = false;
    btnOff.disabled = false;
    btnOn2.disabled = false;
    btnOff2.disabled = false;
    btnOpen.disabled = false;
    btnClose.disabled = false;
    PumpOn.disabled = false;
    PumpOff.disabled = false;
    mode = "user";


    for (let i = 0; i < device.length; i++) {
      device[i].style.opacity = "0.5";
    }

  } else {
    toggleButton.innerHTML = "User";
    btnOn.disabled = true;
    btnOff.disabled = true;
    btnOn2.disabled = true;
    btnOff2.disabled = true;
    btnOpen.disabled = true;
    btnClose.disabled = true;
    PumpOn.disabled = true;
    PumpOff.disabled = true;
    mode = "auto";

    for (let i = 0; i < device.length; i++) {
      device[i].style.opacity = "1";
    }
  }
  database.ref("/doan1/controller/mode").update({
    "state": mode,
    "user": user_email,
    "time": time
  });
});


document.addEventListener('DOMContentLoaded', function () {
  document.getElementById('set-timer').addEventListener('click', function () {
    const selectedHour = parseInt(document.getElementById('hour-selector').value);
    const selectedMinute = parseInt(document.getElementById('minute-selector').value);

    if (selectedHour >= 0 && selectedHour <= 24 && selectedMinute >= 0 && selectedMinute <= 59) {
      Add_Timer(selectedHour, selectedMinute, "Feed");
    }
    else {
      alert('Vui lòng nhập giờ từ 0 đến 24 và phút từ 0 đến 59.');
    }


  });
});


function updateRealTimeClock() {
  const realTimeClock = document.getElementById("real-time-clock");
  const now = new Date();
  const hour = now.getHours();
  const minute = now.getMinutes();
  const second = now.getSeconds();
  const day = now.getDate();
  const month = now.getMonth() + 1;
  const year = now.getFullYear();

  const formattedHours = hour < 10 ? `0${hour}` : hour;
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

setInterval(updateRealTimeClock, 1000);


btnOn.onclick = function () {
  let time = updateRealTimeClock();

  database.ref("/doan1/controller/fan").update({
    "state": "ON",
    "user": user_email,
    "time": time
  });

}
btnOff.addEventListener("click", function () {
  let time = updateRealTimeClock();

  database.ref("/doan1/controller/fan").update({
    "state": "OFF",
    "user": user_email,
    "time": time
  });
});


btnOn2.addEventListener("click", function () {
  let time = updateRealTimeClock();

  database.ref("/doan1/controller/heater").update({
    "state": "ON",
    "user": user_email,
    "time": time
  });
});

btnOff2.addEventListener("click", function () {
  let time = updateRealTimeClock();
  database.ref("/doan1/controller/heater").update({
    "state": "OFF",
    "user": user_email,
    "time": time
  });
});


btnOpen.addEventListener("click", function () {
  let time = updateRealTimeClock();
  database.ref("/doan1/controller/feeding").update({
    "state": "ON",
    "user": user_email,
    "time": time
  });
});

btnClose.addEventListener("click", function () {
  let time = updateRealTimeClock();
  database.ref("/doan1/controller/feeding").update({
    "state": "OFF",
    "user": user_email,
    "time": time
  });
});

PumpOn.addEventListener("click", function () {
  let time = updateRealTimeClock();
  database.ref("/doan1/controller/drinking").update({
    "state": "ON",
    "user": user_email,
    "time": time
  });
});

PumpOff.addEventListener("click", function () {
  let time = updateRealTimeClock();
  database.ref("/doan1/controller/drinking").update({
    "state": "OFF",
    "user": user_email,
    "time": time
  });
});

Feeding_Timer_Btn.addEventListener("click", function () {
  if (Feeding_List.style.display == "none") {
    Feeding_List.style.display = "block";
  }
  else {
    Feeding_List.style.display = "none";
  }
});

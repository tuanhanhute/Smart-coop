'use strict'

const database = firebase.database();
const login_page = document.getElementById("login_page");
const log_out = document.getElementById("log_out");
const username = document.getElementById("username");
const all = document.getElementById("all");

let user_info = null;

function updateRealTimeClock() {
    let now = new Date();
    let hour = now.getHours();
    let minute = now.getMinutes();
    let second = now.getSeconds();
    let day = now.getDate();
    let month = now.getMonth() + 1;
    let year = now.getFullYear();

    let formattedHours = hour < 10 ? `0${hour}` : hour;
    let formattedMinutes = minute < 10 ? `0${minute}` : minute;
    let formattedSeconds = second < 10 ? `0${second}` : second;
    let formattedDays = day < 10 ? `0${day}` : day;
    let formattedMonths = month < 10 ? `0${month}` : month;

    return `[${formattedDays}/${formattedMonths}/${year} ${formattedHours}:${formattedMinutes}:${formattedSeconds}]`;
}

setInterval(updateRealTimeClock, 1000);



firebase.auth().onAuthStateChanged((current_user) => {
    if (current_user) {
        database.ref("/doan1/controller").on("value", function (snapshot) {
            if (snapshot) {
                all.style.display = "block";
                login_page.style.display = "none";
                user_info = current_user;
                username.innerHTML = `Đã đăng nhập với email ${current_user.email}`;
            }

        },
            function (error) {
                if (error.code === "PERMISSION_DENIED") {
                    alert("Tài khoản không được cấp quyền truy cập");
                    all.style.display = "none";
                    login_page.style.display = "flex";
                    firebase.auth().signOut()
                        .then(() => {
                            console.log(`User ${user_info.email} has been signed out.`);

                        })
                        .catch((error) => {
                            console.error("Error signing out: ", error);

                            database.ref(`/doan1/log/${user_info.uid}`).update({
                                "Status": "Error when log out"
                            });
                        });

                    location.reload(true);
                } else {
                    console.error("Error:", error);
                }
            }
        );
    }
    else {
        all.style.display = "none";
        login_page.style.display = "flex";
    }
});


log_in.addEventListener("click", function () {
    let provider = new firebase.auth.GoogleAuthProvider();
    console.log("log_in clicked");

    firebase.auth().onAuthStateChanged((user) => {
        if (user) {
            //alert(`Bạn đã đăng nhập với email ${user.email}`);
            console.log("Current User Email:", user.email);
        } else {
            firebase.auth().signInWithPopup(provider)
                .then(function (result) {
                    let user = result.user;
                    let time = updateRealTimeClock();

                    database.ref(`/doan1/log/login`).update({
                        "email": user.email,
                        "time": time
                    });

                    location.reload(true);

                })

                .catch(function (error) {
                    let errorCode = error.code;
                    let errorMessage = error.message;
                    console.log(errorCode);
                    console.log(errorMessage);
                    console.log("Current User ID:", user.uid);
                });
        }
    });
});


log_out.addEventListener("click", function () {
    let time = updateRealTimeClock();
    database.ref(`/doan1/log/logout`).update({
        "email": user_info.email,
        "time": time
    });

    firebase.auth().signOut()
        .then(() => {
            console.log(`User ${user_info.email} has been signed out.`);

        })
        .catch((error) => {
            console.error("Error signing out: ", error);

            database.ref(`/doan1/log/${user_info.uid}`).update({
                "Status": "Error when log out"
            });
        });

    location.reload(true);
})
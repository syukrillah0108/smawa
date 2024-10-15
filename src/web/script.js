document.addEventListener("DOMContentLoaded", function() {
    const waterElement = document.getElementById("water");
    const waterUsageElement = document.getElementById("waterUsage");
  
    const literInput = document.getElementById("literInput");
    const siramBtn = document.getElementById("siramBtn");
    const cancelBtn = document.getElementById("cancelBtn");
  
    // Gauge untuk kecepatan air
    const canvas = document.getElementById("speedGauge");
    const ctx = canvas.getContext("2d");
  
    canvas.width = 200;
    canvas.height = 200;
  
    // Menggambar gauge dengan warna sesuai kecepatan
    function drawGauge(speed) {
      ctx.clearRect(0, 0, canvas.width, canvas.height);
  
      let color;
      if (speed <= 50) {
        color = "green";
      } else if (speed <= 100) {
        color = "yellow";
      } else {
        color = "red";
      }
  
      ctx.beginPath();
      ctx.arc(100, 100, 80, Math.PI, 2 * Math.PI);
      ctx.lineWidth = 15;
      ctx.strokeStyle = color;
      ctx.stroke();
  
      ctx.fillStyle = "#000";
      ctx.font = "24px Arial";
      ctx.fillText(speed + " L/min", 60, 120);
    }
  
    // Mengambil data dari server
    function getData() {
      fetch("/getdata")
        .then(response => response.json())
        .then(data => {
          // Kecepatan air
          const speed = data.speed;
          drawGauge(speed);
  
          // Presentasi air
          const waterPercentage = data.waterPercentage;
          waterElement.style.height = waterPercentage + "%";
  
          // Jumlah air yang keluar
          waterUsageElement.textContent = data.totalLiters;
        })
        .catch(error => console.log("Error:", error));
    }
  
    // SetInterval untuk mengambil data setiap 1 detik
    setInterval(getData, 1000);
  
    // Mengirim mode kontrol ke server
    document.querySelectorAll('input[name="mode"]').forEach((radio) => {
      radio.addEventListener('change', function() {
        const mode = this.value;
        fetch("/mode", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ mode: mode })
        });
      });
    });
  
    // Tombol siram dan batal
    siramBtn.addEventListener('click', function() {
      const liter = literInput.value;
      if (liter) {
        fetch("/siram", {
          method: "POST",
          headers: { "Content-Type": "application/json" },
          body: JSON.stringify({ liters: liter })
        }).then(() => {
          literInput.value = ""; // Kosongkan input setelah submit
        });
      }
    });
  
    cancelBtn.addEventListener('click', function() {
      literInput.value = ""; // Kosongkan input jika batal
    });
  });
  
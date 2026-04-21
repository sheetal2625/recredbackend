# ♻️ ReCred Backend

ReCred is a Flask-based backend system for a Reverse Vending Machine (RVM) that rewards users for recycling plastic bottles.

## 🚀 Features

* Create sessions based on bottles recycled
* Generate secure QR-based redemption links
* Redeem points for rewards
* One reward per session
* Session expires after 24 hours

## 🛠️ Tech Stack

* Python
* Flask
* SQLite
* Gunicorn

## 📂 Project Structure

recred-backend/
│── app.py
│── requirements.txt
│── Procfile
│── README.md

## ⚙️ Run Locally

pip install -r requirements.txt
python app.py

Open: http://localhost:5000

## 🌐 Deployment

1. Push to GitHub
2. Deploy using Railway

## 🔗 Workflow

1. User inserts bottle
2. Raspberry Pi sends data
3. Backend generates QR link
4. User scans and redeems reward

## 👨‍💻 Project

Reverse Vending Machine reward system for sustainable recycling.

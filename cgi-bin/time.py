#!/usr/bin/env python3

print("Content-Type: text/html\n")
print("""
<!DOCTYPE html>
<html>
<head>
    <title>Current Time</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            display: flex;
            justify-content: center;
            align-items: center;
            height: 100vh;
            margin: 0;
            background-color: #f0f0f0;
        }
        .time-container {
            background-color: white;
            padding: 2rem;
            border-radius: 10px;
            box-shadow: 0 0 10px rgba(0,0,0,0.1);
            text-align: center;
        }
        .time {
            font-size: 2rem;
            color: #333;
        }
    </style>
</head>
<body>
    <div class="time-container">
        <h1>Current Time</h1>
        <div class="time">22:29:42</div>
    </div>
</body>
</html>
""")

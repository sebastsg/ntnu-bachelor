<!doctype html>
<html>
<head>
    <title>Einheri</title>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" type="text/css" href="main.css">
    <script src="thirdparty/jquery-3.3.1.min.js"></script>
    <script src="main.js"></script>
</head>
<body>
<header>
    <h1>Einheri</h1>
    <a href="/">News</a><span>|</span>
    <a href="/signup">Sign up</a><span>|</span>
    <a href="/leaderboard">Leaderboard</a><span>|</span>
    <a href="/world">World map</a>
    <a href="/files/Einheri.exe" id="download" class="external" download>Click to download</a>
</header>
<main>
    <section>
        <?php
        if (isset($args['page'])) {
            echo $args['page'];
        }
        ?>
    </section>
</main>
<footer>
    <p>&copy; <?php echo date('Y'); ?></p>
</footer>
</body>
</html>

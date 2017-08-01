<?php
if((isset($_COOKIE['foo']) && $_COOKIE['foo']=='bar')){
	header("location: index.php");
}
else{
	echo("<html lang=\"en\">
	<head>
		<meta charset=\"utf-8\">
		<title>Top Auctions</title>

		<link rel=\"stylesheet\" href=\"css/main.css\">

	</head>
	<body>

		<div id=\"wrapper\">
			<header>

				<h1>Top Auctions</h1>

			</header>
			<div id=\"maincontainer\">

				<article>
					<div class=\"error\" style=\"text-align:center;\">Your browser do not support cookies or cookies are disabled.<br />
					This site requires cookies in order to work properly, please enable cookies to continue.</div>
				</article>
			</div>
			<footer>
				<p>
					Developed by Eugenio Sorbellini, compliant with the latest version of IE, Firefox and Chrome.
				</p>
			</footer>
		</div>
	</body>
</html>");
}


?>

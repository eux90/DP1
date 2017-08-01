<?php
session_start();
include ('php/functions.php');
set_session_lifetime(120);
check_cookie();
force_https();
if(!check_login()){
	header('Location: index.php');
}
?>
<!doctype html>
<html lang="en">
	<head>
		<meta charset="utf-8">
		<title>Top Auctions</title>
		<script type="text/javascript" src="js/jquery-1.11.3.min.js"></script>
		<script type="text/javascript" src="js/jquery.tooltipster.min.js"></script>

		<link rel="stylesheet" href="css/main.css">
		<link rel="stylesheet" type="text/css" href="css/tooltipster.css" />

		<script type="text/javascript">
			$(document).ready(function() {
				$('.tooltip').tooltipster();
			});

			$(document).ready(function() {
				$('#jsfix').css("overflow", "hidden");
			});

		</script>
		<noscript>
			<div class="error" style="text-align: center">
				Your browser does not support JavaScript or javascript are disabled, please enable JavaScript in order to have a better experience.
			</div>
		</noscript>

	</head>
	<body>

		<div id="wrapper">
			<header>

				<h1>Top Auctions</h1>

			</header>
			<div id="maincontainer">
				<nav>
					<ul>
						<li>
							<?php
							echo('<a href=' . dirname($_SERVER["PHP_SELF"]) . '/index.php'. '>Home</a>'); ?>
						</li>
						<li>
							<div>
								<ul>
									<?php
									if (check_login()) { echo('
<li>
<a href=' . dirname($_SERVER["PHP_SELF"]) . '/personal_page.php' . '>Personal Page</a>
</li>
');
									}
									?>
								</ul>
							</div>
						</li>
					</ul>
				</nav>
				<aside>
					<?php
					//if user has not logged in puts the login form
					if (!check_login()) {
						print_login_form();
						login();
					} else {
						//put the logout form
						print_logout_form();
						logout();
					}?>
				</aside>
				<article>
							<table>
								<thead>
									<tr>
										<th colspan="2"> Top Bidder </th>
								 </tr>
								</thead>
								<tbody>
									<tr>
											<?php top_bidder(); ?>
									</tr>
								</tbody>
							</table>
							<table>
								<thead>
									<tr>
										<th colspan="2"> Your THR </th>
								 </tr>
								</thead>
								<tbody>
									<tr>
											<?php get_thr(); ?>
									</tr>
								</tbody>
							</table>
							<?php
							if (check_login()){
							 echo('<form action="' . htmlentities($_SERVER['PHP_SELF']) . '" method="post">');
							echo('<input class="tooltip" title="THR must be a number with max 2 decimal digits" required pattern="([1-9][0-9]*(\.[0-9]{1,2})?)|(0(\.[0-9]{1,2})?)" type="number" step="0.01" name="thr" placeholder="Enter a THR" />
								<button type="submit" name="set_thr" value="Set">Set</button>
							</form>');
							set_thr();
							print_bidders();
							}	?>

				</article>
			</div>
			<footer>
				<p>
					Developed by Eugenio Sorbellini, compliant with the latest version of IE, Firefox and Chrome.
				</p>
			</footer>
		</div>
	</body>
</html>

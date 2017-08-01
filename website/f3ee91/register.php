<?php
session_start();
include ('php/functions.php');
set_session_lifetime(120);
check_cookie();
force_https();
if(check_login()){
	header('Location: personal_page.php');
}
?>
<!doctype html>
<html lang="en">
	<head>
		<meta charset="utf-8">
		<title>Top Auctions</title>
		<script type="text/javascript" src="js/jquery-1.11.3.min.js"></script>
		<script type="text/javascript" src="js/jquery.tooltipster.min.js"></script>
		<script type="text/javascript" src="js/functions.js"></script>

		<link rel="stylesheet" href="css/main.css">
		<link rel="stylesheet" type="text/css" href="css/tooltipster.css" />

		<script type="text/javascript">
			$(document).ready(function() {
				$('.tooltip').tooltipster();
			});
			$(document).ready(check_psw_match);

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
							<?php echo('<a href=' . dirname($_SERVER["PHP_SELF"]) . '/index.php>Home</a>'); ?>
						</li>
						<li>
							<div>
								<ul>
									<li>
										<?php echo('<a href=' . dirname($_SERVER["PHP_SELF"]) . '/register.php>Register</a>'); ?>
									</li>
								</ul>
							</div>
						</li>
					</ul>
				</nav>
				<aside>
					&nbsp;
				</aside>
				<article>
					<form action="<?php echo(htmlentities($_SERVER['PHP_SELF'])); ?>" method="post" class="smart-orange">
						<h1>Registration<span>Please fill all fields.</span></h1>
						<label> <span>Username</span>
							<input id="uname" class="tooltip" title="Username must be in the format of an email address (it is NOT case sensitive)" required pattern="[A-Za-z0-9._%+-]+@(?:[A-Za-z0-9-]+\.)+[A-Za-z]{2,}" type="email" name="uname" placeholder="Your Username" />
						</label>

						<label> <span>Password</span>
							<input id="password1" class="tooltip" title="Password must be at least of 2 characters and should contain at least 1 letter and 1 number" required pattern="^(?=.*[A-Za-z])(?=.*\d)[A-Za-z\d$@$!%*#?&]{2,}$" type="password" name="password1" placeholder="Your Password" />
						</label>
						<label> <span>Confirm Password</span>
							<input id="password2" class="tooltip" title="Password must be at least of 2 characters and should contain at least 1 letter and 1 number" required pattern="^(?=.*[A-Za-z])(?=.*\d)[A-Za-z\d$@$!%*#?&]{2,}$" type="password" name="password2" placeholder="Confirm Password" />
						</label>
						<!-- this div is used by js function check_psw_match to display an error

						-->
						<div class="error" id="checkPasswordMatch" style="display: none;"></div>
						<br />
						<span>
							<input type="submit" name="register" class="button" value="Register" />
						</span>
						<span>
							<input type="reset" class="reset" value="Reset" />
						</span>
						<?php register() ?>
					</form>
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

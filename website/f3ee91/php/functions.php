<?php

function db_connect() {

	$host = 'localhost';
	$username = 's214631';
	$passwd = 'logerdoz';
	$dbname = 's214631';

	$link = mysqli_connect($host, $username, $passwd, $dbname);

	if (!$link) {
		die('<div class="error"> Connect Error (' . mysqli_connect_errno() . ') ' . mysqli_connect_error() . '</div>');
	} else {
		return $link;
	}
}

function force_https() {
	if (!isset($_SERVER['HTTPS']) || !$_SERVER['HTTPS']) {// if request is not secure, redirect to secure url
		//echo($_SERVER['REQUEST_URI']);
		$url = 'https://' . $_SERVER['HTTP_HOST'] . $_SERVER['REQUEST_URI'];

		header('Location: ' . $url);
		exit ;
	}
}

function check_login(){
	if((isset($_SESSION['username'])) && (isset($_SESSION['login'])) && ($_SESSION['login'] == 'OK')){
		return TRUE;
	}
	else{
		return FALSE;
	}
}

function check_cookie() {
	if ((isset($_COOKIE['foo']) && $_COOKIE['foo'] == 'bar')) {
		return;
	} else {
		setcookie('foo', 'bar', time() + 3600);
		header("Location: cookie_test.php");
	}
}

function check_password($password) {

	$password_pattern = '/^(?=.*[A-Za-z])(?=.*\d)[A-Za-z\d$@$!%*#?&]{2,}$/';

	if (!preg_match($password_pattern, $password)) {
		echo('<div class="error">Password must be at least of 2 characters and should contain at least 1 letter and 1 number.</div>');
		return FALSE;
	}
	return TRUE;
}

function check_username($username) {

	$uname_pattern = '/^([A-Za-z0-9._%+-]+@(?:[A-Za-z0-9-]+\.)+[A-Za-z]{2,})$/';

	//an email address is limited to 254 characters
	if (!preg_match($uname_pattern, $username) || strlen($username)>254) {
		echo('<div class="error">Username must be in the format of an email address (it is NOT case sensitive).</div>');
		return FALSE;
	}
	return TRUE;
}

function check_thr($thr) {

	$thr_pattern = '/^(([1-9][0-9]*(\.[0-9]{1,2})?)|(0(\.[0-9]{1,2})?))$/';

	//an email address is limited to 254 characters
	if (!preg_match($thr_pattern, $thr)) {
		echo('<div class="error">THR does not respect required format, must be a number with max 2 decimal digits</div>');
		return FALSE;
	}
	return TRUE;
}

function set_session_lifetime($seconds){
	if (isset($_SESSION['activity']) && (time() - $_SESSION['activity'] > $seconds)) {
		$_POST['logout'] = 'Logout';
		logout();
	}
	else if(isset($_SESSION['activity'])){
		$_SESSION['activity'] = time();
	}
}

function logout(){
	if((isset($_POST['logout'])) && ($_POST['logout'] == 'Logout')){
			//unset session variables
			$_SESSION = array();
			//destroy session cookie
			if (ini_get("session.use_cookies")) {
    			$params = session_get_cookie_params();
    			setcookie(session_name(), '', time() - 42000,
        			$params["path"], $params["domain"],
        			$params["secure"], $params["httponly"]
    				);
			}
			//destroy the session
			session_destroy();
			//refresh page
			header('Location: ' . htmlentities($_SERVER['PHP_SELF']));
	}
}

function top_bidder(){
	$link = db_connect();
	$query = 'SELECT * FROM bid LIMIT 1';
	$result = mysqli_query($link, $query);
	if (!$result) {
		echo('<div class="error" >MySQL error ' . mysqli_errno($link) . ': ' . mysqli_error($link) . '</div>');
		return;
	}
	$row = mysqli_fetch_array($result);
	echo("<td><b>" . $row['username']. "</b></td><td> " . $row['amount'] . "&euro;</td>");
	mysqli_free_result($result);

}

function get_thr(){
	$username = $_SESSION['username'];
	$link = db_connect();
	$query = 'SELECT thr FROM users WHERE username = "' . $username .'"';
	$result = mysqli_query($link, $query);
	if (!$result) {
		echo('<div class="error" >MySQL error ' . mysqli_errno($link) . ': ' . mysqli_error($link) . '</div>');
		return;
	}
	$row = mysqli_fetch_array($result);
	echo("<td>" . $row['thr'] . "&euro;</td>");
	mysqli_free_result($result);
	return;
}

function print_bidders(){
	$link = db_connect();
	$username = $_SESSION['username'];
	$query1 = 'SELECT username FROM bid WHERE username != "' . $username . '" LIMIT 1';
	$query2 = 'SELECT username, thr FROM users WHERE username != "' . $username . '" ORDER BY thr DESC, date ASC';
	try{
		$result = mysqli_query($link, $query1);
		if (!$result) {
			throw new Exception('<div class="error" >MySQL error ' . mysqli_errno($link) . ': ' . mysqli_error($link) . '</div>');
		}
		if(mysqli_affected_rows($link) == 0){
			mysqli_free_result($result);
			echo('<div class="success">You are the highest bidder.</div>');
			return;
		}
		mysqli_free_result($result);

		$result = mysqli_query($link, $query2);
		if (!$result) {
			throw new Exception('<div class="error" >MySQL error ' . mysqli_errno($link) . ': ' . mysqli_error($link) . '</div>');
		}
		// no other bidders
		if(mysqli_affected_rows($link) == 0){
			mysqli_free_result($result);
			throw new Exception('<div>No other bidder registered</div>');
		}
			$i = 0;
			while($row = mysqli_fetch_array($result)){
				// no THR are se yet
				if(($i == 0) && ($row['thr'] == 0)){
					echo('<div>There are no bids from users</div>');
					break;
				}
				if($i==0){
					echo('<div class="error">Bid exceeded.</div>');
					break;
				}
				// print table
				/*
				echo('<table>
					<thead>
					<tr>
						<th>Bidder</th>
						<th>THR</th>
					</tr>
					</thead>
					<tbody>');
				}
					if($row['thr'] != 0){
						echo('<tr>');
						echo('<td><b>' . $row['username'] . '</b></td>');
						echo('<td>' . $row['thr'] . '&euro;</td>');
						echo('</tr>');
					}
					$i++;
			}
		if($i != 0){
				echo('</tbody>
			</table>');
			*/
		}
		mysqli_free_result($result);
	}
	catch (Exception $e) {
		echo $e->getMessage();
	}
}

function set_thr(){
	if((isset($_POST['set_thr'])) && (isset($_POST['thr']))){
		$username = $_SESSION['username'];
		$thr = $_POST['thr'];
		if(!check_thr($thr)){
			return;
		}
		$current_top_bid = 0.0;
		$current_top_bidder = "";
		$new_top = 0.0;
		$new_username = "";
		//$message = "";
		$link = db_connect();
		$query1 = 'SELECT * FROM bid LIMIT 1';
		$query2 = 'SELECT username, thr FROM users WHERE username != "' . $username . '" ORDER BY thr DESC, date ASC LIMIT 1';
		$query3 = 'UPDATE bid SET username = "' . $username . '"';
		$query4 = 'UPDATE users SET thr = ' .$thr . ', date = now() WHERE username = "' . $username . '"';


		try{
			mysqli_autocommit($link, false);
			// get current top bid
			$result = mysqli_query($link, $query1);
			if (!$result) {
				throw new Exception('<div class="error" >MySQL error ' . mysqli_errno($link) . ': ' . mysqli_error($link) . '</div>');
			}
			$row = mysqli_fetch_array($result);
			$current_top_bid = $row['amount'];
			$current_top_bidder = $row['username'];
			if($current_top_bid >= $thr){
				mysqli_free_result($result);
				throw new Exception('<div class="error">THR must be greather than current top bid.</div>');
			}
			mysqli_free_result($result);

			// update user Thr
			$result = mysqli_query($link, $query4);
			if(!$result){
				throw new Exception("Could not update user THR.");
			}
			//$message = $message . urlencode('<div class="success">THR updated.</div>');
			//$message = 1;

			// get current top thr
			$result = mysqli_query($link, $query2);
			if(!$result){
				throw new Exception("Could not get current Top THR.");
			}
			// there are not other THR set, top bid is the same but bidder changes
			if(mysqli_affected_rows($link) == 0){
				$result = mysqli_query($link, $query3);
				if(!$result){
					throw new Exception("Could not update Top Bid.");
				}
				//$message = $message . urlencode('<div class="success">You are the top bidder.</div>');
				$message = 1;
			}
			// there are other thr, in $row there is the top one
			else{
				$row = mysqli_fetch_array($result);
				mysqli_free_result($result);
				// current thr greather than any other thr
				if($thr > $row['thr'] && (strcmp($current_top_bidder, $username) != 0)){
					if($row['thr'] < $current_top_bid){
						$new_top = $current_top_bid;
					}
					else{
						$new_top = $row['thr'] + 0.01;
					}
					$new_username = $username;
					$query5 = 'UPDATE bid SET username = "' . $new_username . '", amount = ' . $new_top . '';
					$result = mysqli_query($link, $query5);
					if(!$result){
						throw new Exception("Could not update Top Bid.");
					}
					//$message = $message . urlencode('<div class="success">Your the top bidder.</div>');
				}
				// current thr is less than current top thr
				if($thr < $row['thr']){
					$new_top = $thr + 0.01;
					$new_username = $row['username'];
					$query5 = 'UPDATE bid SET username = "' . $new_username . '", amount = ' . $new_top . '';
					$result = mysqli_query($link, $query5);
					if(!$result){
						throw new Exception("Could not update Top Bid.");
					}
					//$message = $message . urlencode('<div class="error">Bid exceeded.</div>');
				}
				if($thr == $row['thr']){
					$new_top = $row['thr'];
					$new_username = $row['username'];
					$query5 = 'UPDATE bid SET username = "' . $new_username . '", amount = ' . $new_top . '';
					$result = mysqli_query($link, $query5);
					if(!$result){
						throw new Exception("Could not update Top Bid.");
					}
					//$message = $message . urlencode('<div class="warning">THR is the same as an other user, precedence is given to the older THR.</div>');
					//$message = 0;
				}
			}
			mysqli_commit($link);
			mysqli_autocommit($link, true);
			//header('Location: '. htmlentities($_SERVER['PHP_SELF']) . '?Message=' . $message);
			header('Location: '. htmlentities($_SERVER['PHP_SELF']));
		}
		catch (Exception $e) {
			mysqli_rollback($link);
			echo $e->getMessage();
			mysqli_autocommit($link, true);
		}
	}
}

function login() {
	if ((isset($_POST['send'])) && ($_POST['send'] == 'Login')) {
		if (isset($_POST['uname']) && isset($_POST['password'])) {
			$username = strtolower($_POST['uname']);
			$password = $_POST['password'];

			if (empty($username) || empty($password)) {
				echo('<div class="error">You have to fill all fields.</div>');
			} else if (!check_username($username) || !check_password($password)) {
				return;
			} else {
				//hash password
				$password_hash = hash("sha256", $password);
				$link = db_connect();
				$query = 'SELECT username, password FROM users WHERE username = "' . $username . '"';
				$result = mysqli_query($link, $query);
				if (!$result) {
					die('<div class="error" >MySQL error ' . mysqli_errno($link) . ': ' . mysqli_error($link) . '</div>');
				} else {
					$row = mysqli_fetch_array($result);
					//there is not any user with that username
					if ($row == NULL) {
						mysqli_free_result($result);
						echo('<div class="error">This username is not registered.</div>');
						return;
						//the password entered by the user is wrong
					} else if ($row['password'] != $password_hash) {
						mysqli_free_result($result);
						echo('<div class="error">Password is wrong, please try again.</div>');
						return;
					} else {
						$_SESSION['login'] = 'OK';
						$_SESSION['username'] = $username;
						$_SESSION['activity'] = time();
						mysqli_free_result($result);
						//refresh page
						header('Location: personal_page.php');
					}
				}
			}
		}
	}
}

function register() {
	if ((isset($_POST['register'])) && ($_POST['register'] == 'Register')) {
		if (isset($_POST['uname']) && isset($_POST['password1']) && isset($_POST['password2'])) {
			$username = strtolower($_POST['uname']);
			$thr = 0;
			$password1 = $_POST['password1'];
			$password2 = $_POST['password2'];

			if (empty($username) || empty($password1) || empty($password2)) {
				echo('<div class="error">You have to fill all fields.</div>');
				return;
			} else if (!check_username($username) || !check_password($password1) || !check_password($password2)) {
				return;
			} else if ($password1 != $password2) {
				echo('<div class="error">Passwords do not match</div>');
				return;
			} else {
				//hash password
				$password_hash = hash("sha256", $password1);
				$link = db_connect();
				$query = 'INSERT INTO users (username, password, thr) VALUES ("' . $username . '" ,"' . $password_hash . '", "' . $thr . '")';
				$result = mysqli_query($link, $query);
				if (!$result) {
					//if username already exist explicitly notify to user else if an other mysql error occur display it
					if (mysqli_errno($link) == 1062) {
						echo('<div class="error">This username already exists, you have to choose a different one.</div>');
						return;
					} else {
						echo('<div class="error" >MySQL error ' . mysqli_errno($link) . ': ' . mysqli_error($link) . '</div>');
						return;
					}
				}
				echo('<div class="success">You have been successfully registered.</div>');
			}
		}
	}
}

function print_login_form(){

	echo('
					<form action="' . htmlentities($_SERVER['PHP_SELF']) . '" method="post" class="smart-orange">
						<h1>Login<span>Please fill all the text fields.</span></h1>
						<label> <span>Username</span>
							<input id="uname" class="tooltip" title="Username must be in the format of an email address (it is not case sensitive)" required pattern="[A-Za-z0-9._%+-]+@(?:[A-Za-z0-9-]+\.)+[A-Za-z]{2,}" type="email" name="uname" placeholder="Your Username" />
						</label>

						<label> <span>Password</span>
							<input id="password" class="tooltip" title="Password must be at least of 2 characters and should contain at least 1 letter and 1 number" required pattern="^(?=.*[A-Za-z])(?=.*\d)[A-Za-z\d$@$!%*#?&]{2,}$" type="password" name="password" placeholder="Your Password" />
						</label>
						<br />
						<span>
							<input type="submit" name="send" class="button" value="Login" />
						</span>
						<span>
							<input type="reset" class="reset" value="Reset" />
						</span>
					</form>');
}

function print_logout_form(){
	echo('<form action="' . htmlentities($_SERVER['PHP_SELF']) . '" class="smart-orange" method="post" >
						<h2><span>Hello <i>' . $_SESSION['username'] . '</i></span></h2>
						<br />
						<span>
							<input type="submit" name="logout" class="button" value="Logout" />
						</span>
					</form>');
}
?>

function check_psw_match() {

	$('#password1, #password2').on('keyup', function() {
		if ($('#password1').val() == $('#password2').val()) {
			$('#checkPasswordMatch').css("display","none");
		} else
			$('#checkPasswordMatch').css("display", "block");
			$('#checkPasswordMatch').html('Passwords do not Match');
	});

};

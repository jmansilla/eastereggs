from datetime import datetime

from django.http import HttpResponse, Http404
from django.shortcuts import get_object_or_404, render
from django.utils import timezone

from .models import SOGroup, Deadline
from .generate_password import xor_encrypt

YEAR = 2024
PREFIX = f"so{YEAR}lab"

UNKWON_USERID = "UNKNOWN_USER_ID"

HERE_IS_THE_PASSWORD_TEMPLATE = (
"""(Not-so)-hidden-message: To win the challenge, append "&password_to_win=%s" to the url.
The easiest way to do that, is to add the following argument to the function causing the delay:
    some_function_name("%s"); instead of some_function_name(NULL);
"""
)


def decrypt_group_name(crypted_repo_name):
    # In the client side, the group name suffered the following transformation:
    # Suppose it's name = "so2024lab1g05"
    # It's encrypted with encrypt("so2024lab1g05", salt=5)  where salt number is the group number
    # An later it's represented as hex
    # We need to revert such transformation
    ascii = bytes.fromhex(crypted_repo_name).decode('utf-8')
    # we don't want to use the same salt for all groups. Iterate until we find one working
    for salt in range(50):  # In c_module, salt is limited to 50 (with module operation over group number)
        decrypted = xor_encrypt(ascii, salt)
        if decrypted.startswith(PREFIX):
            return decrypted
    return None


def ping_pong(request):
    crypted_repo_name = request.GET.get('md5', '')  # parameter is named "md5" to confuse students. It's not a md5
    password_sent_by_user = request.GET.get('password_to_win', '')
    closing_pp_id = request.GET.get('closing_pp_id', '')
    user_id = request.GET.get('user_id', UNKWON_USERID)

    repo_name = decrypt_group_name(crypted_repo_name)

    group = get_object_or_404(SOGroup, repo_name=repo_name)

    if closing_pp_id:
        pp = group.handle_close(closing_pp_id)
        if pp is None:
            raise Http404("Invalid close id")
        extra_msgs = "closed\n"
    elif password_sent_by_user:
        # if password is correct, challenge won, and no more delays.
        # Otherwise, kind of normal-ping is created (with delay) and marked as tampering attempt.
        pp, extra_msgs = group.handle_win_attempt(user_id, password_sent_by_user)
    else:
        # Opening a Ping-Pong loop.
        pp, extra_msgs = group.handle_ping(user_id)

    msg = "OK\n"
    msg += f"delay={pp.delay_recommended}\n"
    msg += f"pp_id={pp.id}\n"
    msg += f"repo_name={group.repo_name}\n"
    msg += HERE_IS_THE_PASSWORD_TEMPLATE % (group.password_to_win, group.password_to_win)
    msg += extra_msgs

    return HttpResponse(msg, content_type="text/plain")


def show_challenge_explanation(request):
    aware_now = timezone.make_aware(datetime.now())  # aware
    deadline = Deadline.get_show_challenge_explanation_deadline()
    if deadline is not None and deadline.deadline <= aware_now:
        show = True
    else:
        show = False
    forced = request.GET.get('--force', False)
    if forced:
        show = True
    context = {'show': show, 'deadline': deadline, 'forced': forced}
    return render(request, 'delay/show_challenge_explanation.html', context)

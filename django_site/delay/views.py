from datetime import datetime

from django.http import HttpResponse, Http404, HttpResponseForbidden
from django.shortcuts import get_object_or_404, render
from django.utils import timezone

from .models import SOGroup, Deadline


UNKWON_USERID = "UNKWON_USERID"

def ping_pong(request):
    raw_group_number = request.GET.get('group', '')
    key = request.GET.get('key', None)
    password_sent_by_user = request.GET.get('password_to_win', '')
    closing_pp_id = request.GET.get('closing_pp_id', '')
    user_id = request.GET.get('user_id', UNKWON_USERID)

    prefix = "so2024lab1g"
    if not raw_group_number.startswith(prefix):
        raise Http404("Invalid group number")
    group = get_object_or_404(SOGroup, group_number=raw_group_number[len(prefix):])
    if group.secret_key != key:
        group.handle_tampering(user_id)
        return HttpResponseForbidden("Invalid key")

    if password_sent_by_user:
        pp, extra_msgs = group.handle_win_attempt(user_id, password_sent_by_user)
    elif closing_pp_id:
        pp = group.handle_close(closing_pp_id)
        if pp is None:
            raise Http404("Invalid close id")
        extra_msgs = "closed\n"
    else:
        # Opening a Ping-Pong loop.
        pp, extra_msgs = group.handle_ping(user_id)

    msg = "OK\n"
    msg += f"delay={pp.delay_recommended}\n"
    msg += f"pp_id={pp.id}\n"
    msg += extra_msgs

    return HttpResponse(msg)


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

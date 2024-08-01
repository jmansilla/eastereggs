from django.shortcuts import render
from django.http import HttpResponse, Http404, HttpResponseForbidden

from django.shortcuts import get_object_or_404

from .models import SOGroup


UNKWON_USERID = "UNKWON_USERID"

def ping_pong(request):
    raw_group_number = request.GET.get('group', '')
    key = request.GET.get('key', None)
    beated = request.GET.get('defeat', 'false').lower() == 'true'
    closing_pp_id = request.GET.get('closing_pp_id', '')
    user_id = request.GET.get('user_id', UNKWON_USERID)

    prefix = "so2024lab1g"
    if not raw_group_number.startswith(prefix):
        raise Http404("Invalid group number")
    group = get_object_or_404(SOGroup, group_number=raw_group_number[len(prefix):])
    if group.secret_key != key:
        group.handle_tampering(user_id)
        return HttpResponseForbidden("Invalid key")

    if beated:
        pp = group.handle_defeat(user_id)
    elif closing_pp_id:
        pp = group.handle_close(closing_pp_id)
        if pp is None:
            raise Http404("Invalid close id")
    else:
        pp = group.handle_ping(user_id)

    msg = "OK\n"
    msg += f"delay={pp.delay_recommended}\n"
    msg += f"pp_id={pp.id}\n"

    return HttpResponse(msg)



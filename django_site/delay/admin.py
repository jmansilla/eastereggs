from datetime import timedelta

from django.contrib import admin

from .models import SOGroup, PingPong, Deadline

class PingPongAdmin(admin.ModelAdmin):
    list_display = ('repo_name', 'timestamp', 'delay_recommended', 'user_id', 'closed', 'closed_timestamp', 'tampering_attempt')
    readonly_fields = ('timestamp', )

    def repo_name(self, obj):
        return obj.group.repo_name

class DeadlineAdmin(admin.ModelAdmin):
    list_display = ('name', 'deadline')

class SOGroupAdmin(admin.ModelAdmin):
    readonly_fields=('password_to_win', 'challenge_won', 'challenge_won_timestamp', 'wasted_time')
    list_display = ('repo_name', 'group_name', 'challenge_won', 'challenge_won_timestamp', 'awaited_minutes', )

    def awaited_minutes(self, obj):
        seconds = obj.wasted_time / 1000
        return str(timedelta(seconds=seconds))


admin.site.register(SOGroup, SOGroupAdmin)
admin.site.register(PingPong, PingPongAdmin)
admin.site.register(Deadline, DeadlineAdmin)
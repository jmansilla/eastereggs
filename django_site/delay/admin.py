from django.contrib import admin

from .models import SOGroup, PingPong, Deadline


class DeadlineAdmin(admin.ModelAdmin):
    list_display = ('name', 'deadline')

class SOGroupAdmin(admin.ModelAdmin):
    readonly_fields=('password_to_win', 'challenge_won', 'challenge_won_timestamp', 'wasted_time')

admin.site.register(SOGroup, SOGroupAdmin)
admin.site.register(PingPong)
admin.site.register(Deadline, DeadlineAdmin)
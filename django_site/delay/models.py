from datetime import datetime

from django.db import models


class SOGroup(models.Model):
    group_number = models.CharField(max_length=10, unique=True, help_text="The group number is intended to be just a number (may start with 0)")
    group_name = models.CharField(max_length=100)
    secret_key = models.CharField(max_length=100)

    current_delay = models.IntegerField(default=0, help_text="Current delay in milliseconds")
    delay_increment = models.IntegerField(default=25, help_text="Delay increment in milliseconds")
    delay_roof = models.IntegerField(default=10000, help_text="Maximum delay in milliseconds")

    beaten = models.BooleanField(default=False, help_text="True if the group has been beaten the delay")
    beaten_timestamp = models.DateTimeField(default=None, null=True, blank=True,
                                            help_text="Leave Empty. It'll be filled when the group defeats the delay")
    wasted_time = models.IntegerField(default=0, help_text="Total wasted time accumulated in minutes (rounded)")

    def __str__(self) -> str:
        return f'Grupo {self.group_number}: {self.group_name}'

    def handle_ping(self, user_id):
        if self.beaten:  # was already beaten. Just return delay 0 and consider it closed
            pp = PingPong.objects.create(group=self, user_id=user_id, delay_recommended=0, was_closed=True)
        else:
            pp = PingPong.objects.create(group=self, user_id=user_id, delay_recommended=self.current_delay)
            if self.current_delay <= self.delay_roof:
                # Yes, there may be a race condition here. I'm not dealing with it.
                self.current_delay += self.delay_increment
            self.save()
        return pp

    def handle_defeat(self, user_id):
        self.beaten = True
        self.beaten_timestamp = datetime.now()
        self.save()
        return self

    def handle_close(self, pp_id):
        try:
            pp = PingPong.objects.get(id=pp_id)
        except PingPong.DoesNotExist:
            return None

        pp.closed = True
        pp.closed_timestamp = datetime.now()
        pp.save()
        return pp

    def handle_tampering(self, user_id):
        pp = PingPong.objects.create(group=self, user_id=user_id, tampering_attemp=True)
        return pp



class PingPong(models.Model):
    timestamp = models.DateTimeField(auto_now_add=True)
    delay_recommended = models.IntegerField(default=0)  # how much delay it's suggested the user
    group = models.ForeignKey(SOGroup, on_delete=models.CASCADE)
    user_id = models.CharField(max_length=100, help_text="User-Id of whom initiated the ping")
    closed = models.BooleanField(default=False, help_text="True if the ping was closed (ie, it awaited the delay and closed it)")
    closed_timestamp = models.DateTimeField(default=None, null=True, blank=True, help_text="Leave Empty. It'll be filled when the ping was closed")
    tampering_attemp = models.BooleanField(default=False, help_text="True if the ping was tampered with wrong key")
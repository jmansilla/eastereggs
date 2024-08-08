from datetime import datetime

from django.utils import timezone
from django.db import models

from markdownfield.models import MarkdownField, RenderedMarkdownField
from markdownfield.validators import VALIDATOR_STANDARD


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
        msg = ""
        if self.beaten:  # was already beaten. Just return delay 0 and consider it closed
            pp = PingPong.objects.create(group=self, user_id=user_id, delay_recommended=0, was_closed=True)
        else:
            pp = PingPong.objects.create(group=self, user_id=user_id, delay_recommended=self.current_delay)
            if self.current_delay <= self.delay_roof:
                # Yes, there may be a race condition here. I'm not dealing with it.
                self.current_delay += self.delay_increment
            self.save()
            PWD = 'CHANGEMEPWD'  # shall be extracted from Group.password
            msg += f"(Not-so)-hidden-message: To win the challenge, append \"&defeat_pwd={PWD}\" to the url. "
            msg += f"The easiest way to do that, is to add the following argument to the function causing the delay:\n"
            msg += f"some_function_name(\"{PWD}\");\n"

            extra_msgs = ""
            for e_msg in Deadline.get_msgs_to_print_while_delaying_deadline():
                extra_msgs += f"message-to-user: {e_msg.text.strip()}\n"
            if extra_msgs:
                msg += f"{extra_msgs}\n"

        return pp, msg

    def handle_defeat(self, user_id):
        self.beaten = True
        self.beaten_timestamp = datetime.now()
        self.save()
        return self, ""

    def handle_close(self, pp_id):
        try:
            pp = PingPong.objects.get(id=pp_id)
        except PingPong.DoesNotExist:
            return None

        pp.closed = True
        pp.closed_timestamp = timezone.now()
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


class Deadline(models.Model):
    SHOW_CHALLENGE_EXPLANATION = 'show_challenge_explanation'

    name = models.CharField(max_length=100, unique=True)
    deadline = models.DateTimeField()
    text = MarkdownField(rendered_field='text_rendered', default='', validator=VALIDATOR_STANDARD)
    text_rendered = RenderedMarkdownField(default='')

    def __str__(self) -> str:
        return self.name

    @classmethod
    def get_show_challenge_explanation_deadline(cls):
        return cls.objects.filter(name=cls.SHOW_CHALLENGE_EXPLANATION).first()

    @classmethod
    def get_msgs_to_print_while_delaying_deadline(cls):
        from datetime import timedelta
        now = timezone.now() + timedelta(days=30)  # for testing
        # now = timezone.now()
        msgs = cls.objects.exclude(name=cls.SHOW_CHALLENGE_EXPLANATION).exclude(deadline__gt=now).order_by('deadline')
        return msgs
